#include "editor.h"

#include <algorithm>
#include <stdio.h>

// ---------------------------------------------------------------------------
// Component ownership
//
// A component is owned by the UI tree while attached. While detached (removed
// from the tree by an undo/redo command) it is owned by the last command that
// held it. ComponentOwner refcounts that ownership so Add + Remove commands
// sharing one component free it exactly once.
// ---------------------------------------------------------------------------

ComponentOwner::~ComponentOwner()
{
	if (comp && !attached) bapi_ui_component_destroy(comp);
}

static void push_layout(EditorState &state)
{
	if (state.ui) bapi_ui_layout(state.ui);
}

static void set_default_rect(bapi_ui_component_t comp)
{
	if (!comp) return;
	bapi_rect_t r = {40.0f, 40.0f, 120.0f, 32.0f};
	switch (bapi_ui_component_get_type(comp)) {
	case BAPI_UI_COMPONENT_IMAGE:
	case BAPI_UI_COMPONENT_VIDEO:
	case BAPI_UI_COMPONENT_NINE_PATCH:
	case BAPI_UI_COMPONENT_ANIMATION:
		r.w = 64.0f;
		r.h = 64.0f;
		break;
	case BAPI_UI_COMPONENT_CIRCLE:
	case BAPI_UI_COMPONENT_POLYGON:
		r.w = 40.0f;
		r.h = 40.0f;
		break;
	case BAPI_UI_COMPONENT_LINE:
	case BAPI_UI_COMPONENT_SEPARATOR:
	case BAPI_UI_COMPONENT_BORDER:
		r.h = 10.0f;
		break;
	default:
		break;
	}
	bapi_ui_component_set_rect(comp, r);
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

class AddComponentCmd : public Command {
public:
	AddComponentCmd(bapi_ui_t ui, bapi_ui_component_type_t type, const char *id,
					bapi_ui_component_t parent)
		: ui_(ui), type_(type), id_(id ? id : ""), parent_(parent)
	{
		owner_ = std::make_shared<ComponentOwner>();
	}

	~AddComponentCmd() override = default;

	void Execute(EditorState &state) override
	{
		if (!owner_->comp) {
			owner_->comp = bapi_ui_component_create(type_, id_.c_str());
			if (!owner_->comp) return;
			set_default_rect(owner_->comp);
		}
		if (parent_) {
			bapi_ui_component_add_child(parent_, owner_->comp);
		} else {
			bapi_ui_add_root(ui_, owner_->comp);
		}
		owner_->attached = true;
		state.owner_registry[owner_->comp] = owner_;
		push_layout(state);
	}

	void Undo(EditorState &state) override
	{
		if (!owner_->comp) return;
		if (parent_) {
			bapi_ui_component_remove(owner_->comp);
		} else {
			bapi_ui_remove_root(ui_, owner_->comp);
		}
		owner_->attached = false;
		push_layout(state);
	}

	const char *Name() const override { return "Add Component"; }

	ComponentOwnerPtr Owner() const { return owner_; }

private:
	bapi_ui_t			 ui_;
	bapi_ui_component_type_t type_;
	std::string			 id_;
	bapi_ui_component_t		 parent_;
	ComponentOwnerPtr		 owner_;
};

class RemoveComponentCmd : public Command {
public:
	RemoveComponentCmd(bapi_ui_t ui, bapi_ui_component_t comp,
					   const std::unordered_map<bapi_ui_component_t, ComponentOwnerPtr> &registry)
		: ui_(ui), comp_(comp)
	{
		parent_ = bapi_ui_component_get_parent(comp);
		auto it = registry.find(comp);
		if (it != registry.end()) {
			owner_ = it->second;
		} else {
			owner_		  = std::make_shared<ComponentOwner>();
			owner_->comp  = comp;
			owner_->attached = true;
		}
	}

	void Execute(EditorState &state) override
	{
		if (parent_) {
			bapi_ui_component_remove(comp_);
		} else {
			bapi_ui_remove_root(ui_, comp_);
		}
		if (owner_) owner_->attached = false;
		push_layout(state);
	}

	void Undo(EditorState &state) override
	{
		if (parent_) {
			bapi_ui_component_add_child(parent_, comp_);
		} else {
			bapi_ui_add_root(ui_, comp_);
		}
		if (owner_) owner_->attached = true;
		push_layout(state);
	}

	const char *Name() const override { return "Remove Component"; }

private:
	bapi_ui_t			  ui_;
	bapi_ui_component_t	  comp_;
	bapi_ui_component_t	  parent_;
	ComponentOwnerPtr	  owner_;
};

class MultiRemoveCmd : public Command {
public:
	MultiRemoveCmd(bapi_ui_t ui, const std::vector<bapi_ui_component_t> &comps,
				   const std::unordered_map<bapi_ui_component_t, ComponentOwnerPtr> &registry)
		: ui_(ui)
	{
		for (bapi_ui_component_t comp : comps) {
			if (!comp) continue;
			Entry entry;
			entry.comp	 = comp;
			entry.parent = bapi_ui_component_get_parent(comp);
			entry.index	 = entry.parent ? child_index(entry.parent, comp) : root_index(comp);
			auto it		 = registry.find(comp);
			if (it != registry.end()) {
				entry.owner = it->second;
			} else {
				entry.owner		  = std::make_shared<ComponentOwner>();
				entry.owner->comp  = comp;
				entry.owner->attached = true;
			}
			entries_.push_back(std::move(entry));
		}
	}

	void Execute(EditorState &state) override
	{
		for (Entry &entry : entries_) {
			detach(entry.comp, entry.parent);
			if (entry.owner) entry.owner->attached = false;
		}
		push_layout(state);
	}

	void Undo(EditorState &state) override
	{
		std::vector<Entry> order = entries_;
		std::sort(order.begin(), order.end(),
				  [](const Entry &a, const Entry &b) { return a.index < b.index; });
		for (Entry &entry : order) {
			attach(entry.comp, entry.parent, entry.index);
			if (entry.owner) entry.owner->attached = true;
		}
		push_layout(state);
	}

	const char *Name() const override { return "Delete Selection"; }

private:
	struct Entry {
		bapi_ui_component_t comp;
		bapi_ui_component_t parent;
		int					index;
		ComponentOwnerPtr	owner;
	};

	static int child_index(bapi_ui_component_t parent, bapi_ui_component_t comp)
	{
		for (int i = 0; i < bapi_ui_component_get_child_count(parent); i++)
			if (bapi_ui_component_get_child(parent, i) == comp) return i;
		return -1;
	}

	int root_index(bapi_ui_component_t comp)
	{
		for (int i = 0; i < bapi_ui_get_root_count(ui_); i++)
			if (bapi_ui_get_root(ui_, i) == comp) return i;
		return -1;
	}

	void detach(bapi_ui_component_t comp, bapi_ui_component_t parent)
	{
		if (parent)
			bapi_ui_component_remove(comp);
		else
			bapi_ui_remove_root(ui_, comp);
	}

	void attach(bapi_ui_component_t comp, bapi_ui_component_t parent, int index)
	{
		if (parent) {
			if (bapi_ui_component_insert_child(parent, comp, index) != 0)
				bapi_ui_component_add_child(parent, comp);
		} else {
			if (bapi_ui_insert_root(ui_, comp, index) != 0) bapi_ui_add_root(ui_, comp);
		}
	}

	bapi_ui_t			 ui_;
	std::vector<Entry>	 entries_;
};

class MultiRectCmd : public Command {
public:
	explicit MultiRectCmd(std::vector<RectMove> moves) : moves_(std::move(moves)) {}

	void Execute(EditorState &state) override
	{
		for (const RectMove &move : moves_)
			if (move.comp) bapi_ui_component_set_rect(move.comp, move.new_rect);
		push_layout(state);
	}

	void Undo(EditorState &state) override
	{
		for (const RectMove &move : moves_)
			if (move.comp) bapi_ui_component_set_rect(move.comp, move.old_rect);
		push_layout(state);
	}

	const char *Name() const override { return "Move Components"; }

private:
	std::vector<RectMove> moves_;
};

class SetRectCmd : public Command {
public:
	SetRectCmd(bapi_ui_component_t comp, bapi_rect_t old_rect, bapi_rect_t new_rect)
		: comp_(comp), old_rect_(old_rect), new_rect_(new_rect)
	{
	}

	void Execute(EditorState &state) override
	{
		bapi_ui_component_set_rect(comp_, new_rect_);
		push_layout(state);
	}

	void Undo(EditorState &state) override
	{
		bapi_ui_component_set_rect(comp_, old_rect_);
		push_layout(state);
	}

	const char *Name() const override { return "Move Component"; }

private:
	bapi_ui_component_t comp_;
	bapi_rect_t			old_rect_;
	bapi_rect_t			new_rect_;
};

class SetTextCmd : public Command {
public:
	SetTextCmd(bapi_ui_component_t comp, const char *old_text, const char *new_text)
		: comp_(comp), old_text_(old_text ? old_text : ""), new_text_(new_text ? new_text : "")
	{
	}

	void Execute(EditorState &state) override
	{
		(void)state;
		bapi_ui_component_set_text(comp_, new_text_.c_str());
	}

	void Undo(EditorState &state) override
	{
		(void)state;
		bapi_ui_component_set_text(comp_, old_text_.c_str());
	}

	const char *Name() const override { return "Edit Text"; }

private:
	bapi_ui_component_t comp_;
	std::string			old_text_;
	std::string			new_text_;
};

class SetIdCmd : public Command {
public:
	SetIdCmd(bapi_ui_component_t comp, const char *old_id, const char *new_id)
		: comp_(comp), old_id_(old_id ? old_id : ""), new_id_(new_id ? new_id : "")
	{
	}

	void Execute(EditorState &state) override
	{
		(void)state;
		bapi_ui_component_set_id(comp_, new_id_.c_str());
	}

	void Undo(EditorState &state) override
	{
		(void)state;
		bapi_ui_component_set_id(comp_, old_id_.c_str());
	}

	const char *Name() const override { return "Rename Component"; }

private:
	bapi_ui_component_t comp_;
	std::string			old_id_;
	std::string			new_id_;
};

class SetColorCmd : public Command {
public:
	SetColorCmd(bapi_ui_component_t comp, bapi_ui_color_role_t role, bapi_color_t old_color,
				bapi_color_t new_color)
		: comp_(comp), role_(role), old_color_(old_color), new_color_(new_color)
	{
	}

	void Execute(EditorState &state) override
	{
		(void)state;
		bapi_ui_component_set_color(comp_, role_, new_color_);
	}

	void Undo(EditorState &state) override
	{
		(void)state;
		bapi_ui_component_set_color(comp_, role_, old_color_);
	}

	const char *Name() const override { return "Change Color"; }

private:
	bapi_ui_component_t comp_;
	bapi_ui_color_role_t role_;
	bapi_color_t		old_color_;
	bapi_color_t		new_color_;
};

class SetTextSizeCmd : public Command {
public:
	SetTextSizeCmd(bapi_ui_component_t comp, float old_size, float new_size)
		: comp_(comp), old_size_(old_size), new_size_(new_size)
	{
	}

	void Execute(EditorState &state) override
	{
		(void)state;
		bapi_ui_component_set_text_size(comp_, new_size_);
	}

	void Undo(EditorState &state) override
	{
		(void)state;
		bapi_ui_component_set_text_size(comp_, old_size_);
	}

		const char *Name() const override { return "Change Text Size"; }

private:
	bapi_ui_component_t comp_;
	float				old_size_;
	float				new_size_;
};

// Numeric component field setters (min/max/step/radius). Reads/writes via the
// matching BAPI accessor so undo/redo and XML serialization stay consistent.
static float get_numeric_field(bapi_ui_component_t comp, NumericField field)
{
	switch (field) {
	case NumericField::MinValue:
		return bapi_ui_component_get_min_value(comp);
	case NumericField::MaxValue:
		return bapi_ui_component_get_max_value(comp);
	case NumericField::Step:
		return bapi_ui_component_get_step(comp);
	case NumericField::Radius:
		return bapi_ui_component_get_radius(comp);
	}
	return 0.0f;
}

static void set_numeric_field(bapi_ui_component_t comp, NumericField field, float value)
{
	switch (field) {
	case NumericField::MinValue:
		bapi_ui_component_set_min_value(comp, value);
		break;
	case NumericField::MaxValue:
		bapi_ui_component_set_max_value(comp, value);
		break;
	case NumericField::Step:
		bapi_ui_component_set_step(comp, value);
		break;
	case NumericField::Radius:
		bapi_ui_component_set_radius(comp, value);
		break;
	}
}

class SetNumericCmd : public Command {
public:
	SetNumericCmd(bapi_ui_component_t comp, NumericField field, float old_value, float new_value)
		: comp_(comp), field_(field), old_value_(old_value), new_value_(new_value)
	{
	}

	void Execute(EditorState &state) override
	{
		(void)state;
		set_numeric_field(comp_, field_, new_value_);
	}

	void Undo(EditorState &state) override
	{
		(void)state;
		set_numeric_field(comp_, field_, old_value_);
	}

	const char *Name() const override { return "Change Value"; }

private:
	bapi_ui_component_t comp_;
	NumericField		field_;
	float				old_value_;
	float				new_value_;
};

static bool get_bool_field(bapi_ui_component_t comp, BoolField field)
{
	switch (field) {
	case BoolField::Checked:
		return bapi_ui_component_is_checked(comp) != 0;
	case BoolField::Relative:
		return bapi_ui_component_get_relative(comp) != 0;
	case BoolField::Visible:
		return bapi_ui_component_is_visible(comp) != 0;
	case BoolField::Enabled:
		return bapi_ui_component_is_enabled(comp) != 0;
	}
	return false;
}

static void set_bool_field(bapi_ui_component_t comp, BoolField field, bool value)
{
	switch (field) {
	case BoolField::Checked:
		bapi_ui_component_set_checked(comp, value ? 1 : 0);
		break;
	case BoolField::Relative:
		bapi_ui_component_set_relative(comp, value ? 1 : 0);
		break;
	case BoolField::Visible:
		bapi_ui_component_set_visible(comp, value ? 1 : 0);
		break;
	case BoolField::Enabled:
		bapi_ui_component_set_enabled(comp, value ? 1 : 0);
		break;
	}
}

class SetBoolCmd : public Command {
public:
	SetBoolCmd(bapi_ui_component_t comp, BoolField field, bool old_value, bool new_value)
		: comp_(comp), field_(field), old_value_(old_value), new_value_(new_value)
	{
	}

	void Execute(EditorState &state) override
	{
		(void)state;
		set_bool_field(comp_, field_, new_value_);
	}

	void Undo(EditorState &state) override
	{
		(void)state;
		set_bool_field(comp_, field_, old_value_);
	}

		const char *Name() const override { return "Toggle Property"; }

private:
	bapi_ui_component_t comp_;
	BoolField			field_;
	bool				old_value_;
	bool				new_value_;
};

class SetValueCmd : public Command {
public:
	SetValueCmd(bapi_ui_component_t comp, float old_value, float new_value)
		: comp_(comp), old_value_(old_value), new_value_(new_value)
	{
	}

	void Execute(EditorState &state) override
	{
		(void)state;
		bapi_ui_component_set_value(comp_, new_value_);
	}

	void Undo(EditorState &state) override
	{
		(void)state;
		bapi_ui_component_set_value(comp_, old_value_);
	}

	const char *Name() const override { return "Change Value"; }

private:
	bapi_ui_component_t comp_;
	float				old_value_;
	float				new_value_;
};

class SetColumnsCmd : public Command {
public:
	SetColumnsCmd(bapi_ui_component_t comp, int old_value, int new_value)
		: comp_(comp), old_value_(old_value), new_value_(new_value)
	{
	}

	void Execute(EditorState &state) override
	{
		(void)state;
		bapi_ui_component_set_columns(comp_, new_value_);
	}

	void Undo(EditorState &state) override
	{
		(void)state;
		bapi_ui_component_set_columns(comp_, old_value_);
	}

	const char *Name() const override { return "Change Columns"; }

private:
	bapi_ui_component_t comp_;
	int					old_value_;
	int					new_value_;
};

class SetSidesCmd : public Command {
public:
	SetSidesCmd(bapi_ui_component_t comp, int old_value, int new_value)
		: comp_(comp), old_value_(old_value), new_value_(new_value)
	{
	}

	void Execute(EditorState &state) override
	{
		(void)state;
		bapi_ui_component_set_sides(comp_, new_value_);
	}

	void Undo(EditorState &state) override
	{
		(void)state;
		bapi_ui_component_set_sides(comp_, old_value_);
	}

	const char *Name() const override { return "Change Sides"; }

private:
	bapi_ui_component_t comp_;
	int					old_value_;
	int					new_value_;
};

// ---------------------------------------------------------------------------
// Reparent / clone / align commands
// ---------------------------------------------------------------------------

class ReparentCmd : public Command {
public:
	ReparentCmd(bapi_ui_t ui, bapi_ui_component_t comp, bapi_ui_component_t new_parent)
		: ui_(ui), comp_(comp), new_parent_(new_parent)
	{
		old_parent_ = bapi_ui_component_get_parent(comp);
		old_index_	= child_index(old_parent_, comp);
		if (!new_parent_)
			new_index_ = bapi_ui_get_root_count(ui_);
		else
			new_index_ = bapi_ui_component_get_child_count(new_parent_);
	}

	void Execute(EditorState &state) override
	{
		(void)state;
		detach(comp_, old_parent_);
		attach(comp_, new_parent_, new_index_);
		push_layout(state);
	}

	void Undo(EditorState &state) override
	{
		(void)state;
		detach(comp_, new_parent_);
		attach(comp_, old_parent_, old_index_);
		push_layout(state);
	}

	const char *Name() const override { return "Reparent Component"; }

private:
	static int child_index(bapi_ui_component_t parent, bapi_ui_component_t comp)
	{
		if (!parent) return -1;
		int count = bapi_ui_component_get_child_count(parent);
		for (int i = 0; i < count; i++)
			if (bapi_ui_component_get_child(parent, i) == comp) return i;
		return -1;
	}

	void detach(bapi_ui_component_t comp, bapi_ui_component_t parent)
	{
		if (parent) {
			bapi_ui_component_remove(comp);
		} else {
			bapi_ui_remove_root(ui_, comp);
		}
	}

	void attach(bapi_ui_component_t comp, bapi_ui_component_t parent, int index)
	{
		if (parent) {
			if (bapi_ui_component_insert_child(parent, comp, index) != 0)
				bapi_ui_component_add_child(parent, comp);
		} else {
			bapi_ui_add_root(ui_, comp);
		}
	}

	bapi_ui_t			 ui_;
	bapi_ui_component_t	 comp_;
	bapi_ui_component_t	 old_parent_;
	bapi_ui_component_t	 new_parent_;
	int					 old_index_;
	int					 new_index_;
};

class CloneCmd : public Command {
public:
	CloneCmd(bapi_ui_t ui, ComponentOwnerPtr owner, bapi_ui_component_t parent)
		: ui_(ui), owner_(std::move(owner)), parent_(parent)
	{
	}

	void Execute(EditorState &state) override
	{
		if (!owner_->comp) return;
		if (parent_) {
			bapi_ui_component_add_child(parent_, owner_->comp);
		} else {
			bapi_ui_add_root(ui_, owner_->comp);
		}
		owner_->attached = true;
		state.owner_registry[owner_->comp] = owner_;
		push_layout(state);
	}

	void Undo(EditorState &state) override
	{
		if (!owner_->comp) return;
		if (parent_) {
			bapi_ui_component_remove(owner_->comp);
		} else {
			bapi_ui_remove_root(ui_, owner_->comp);
		}
		owner_->attached = false;
		push_layout(state);
	}

	const char *Name() const override { return "Duplicate Component"; }

private:
	bapi_ui_t		 ui_;
	ComponentOwnerPtr owner_;
	bapi_ui_component_t parent_;
};

class ReorderCmd : public Command {
public:
	ReorderCmd(bapi_ui_t ui, bapi_ui_component_t comp, ReorderOp op)
		: ui_(ui), comp_(comp), parent_(bapi_ui_component_get_parent(comp))
	{
		int count = parent_ ? bapi_ui_component_get_child_count(parent_)
							: bapi_ui_get_root_count(ui_);
		old_index_ = current_index();
		new_index_ = target_index(old_index_, count, op);
	}

	void Execute(EditorState &state) override
	{
		(void)state;
		move_to(new_index_);
		push_layout(state);
	}

	void Undo(EditorState &state) override
	{
		(void)state;
		move_to(old_index_);
		push_layout(state);
	}

	const char *Name() const override { return "Reorder Component"; }

private:
	int current_index() const
	{
		if (parent_) {
			for (int i = 0; i < bapi_ui_component_get_child_count(parent_); i++)
				if (bapi_ui_component_get_child(parent_, i) == comp_) return i;
			return -1;
		}
		for (int i = 0; i < bapi_ui_get_root_count(ui_); i++)
			if (bapi_ui_get_root(ui_, i) == comp_) return i;
		return -1;
	}

	static int target_index(int old_index, int count, ReorderOp op)
	{
		switch (op) {
		case ReorderOp::Up:
			return old_index > 0 ? old_index - 1 : old_index;
		case ReorderOp::Down:
			return old_index < count - 1 ? old_index + 1 : old_index;
		case ReorderOp::Front:
			return 0;
		case ReorderOp::Back:
			return count > 0 ? count - 1 : 0;
		}
		return old_index;
	}

	void move_to(int index)
	{
		if (parent_) {
			if (bapi_ui_component_remove(comp_) != 0) return;
			if (bapi_ui_component_insert_child(parent_, comp_, index) != 0)
				bapi_ui_component_add_child(parent_, comp_);
		} else {
			if (bapi_ui_remove_root(ui_, comp_) != 0) return;
			if (bapi_ui_insert_root(ui_, comp_, index) != 0) bapi_ui_add_root(ui_, comp_);
		}
	}

	bapi_ui_t			 ui_;
	bapi_ui_component_t	 comp_;
	bapi_ui_component_t	 parent_;
	int					 old_index_;
	int					 new_index_;
};

static bool comp_in_list(const std::vector<bapi_ui_component_t> &list, bapi_ui_component_t comp)
{
	return std::find(list.begin(), list.end(), comp) != list.end();
}

static std::string unique_id(EditorState &state, const char *base)
{
	std::string candidate = base ? base : "component";
	candidate += "_copy";
	int			suffix = 2;
	while (bapi_ui_find(state.ui, candidate.c_str()) != nullptr) {
		candidate = std::string(base ? base : "component") + "_copy_" + std::to_string(suffix++);
	}
	return candidate;
}

void EditorReparentComponent(EditorState &state, bapi_ui_component_t comp,
							 bapi_ui_component_t new_parent)
{
	if (!comp || comp == new_parent || EditorIsAncestor(comp, new_parent)) return;
	if (bapi_ui_component_get_parent(comp) == new_parent) return;
	EditorPushCommand(state, std::make_unique<ReparentCmd>(state.ui, comp, new_parent));
}

void EditorCloneComponent(EditorState &state, bapi_ui_component_t source, bapi_ui_component_t parent)
{
	if (!source || !state.ui) return;
	const char *id = bapi_ui_component_get_id(source);
	std::string new_id = unique_id(state, id ? id : "component");
	bapi_ui_component_t clone = bapi_ui_component_clone(source);
	if (!clone) return;
	if (bapi_ui_component_set_id(clone, new_id.c_str()) != 0) {
		bapi_ui_component_destroy(clone);
		return;
	}
	auto owner = std::make_shared<ComponentOwner>();
	owner->comp = clone;
	EditorPushCommand(state, std::make_unique<CloneCmd>(state.ui, owner, parent));
}

void EditorDuplicateSelection(EditorState &state)
{
	if (state.selection_list.empty() || !state.ui) return;
	std::vector<bapi_ui_component_t> sources = state.selection_list;
	for (bapi_ui_component_t comp : sources) {
		if (comp_in_list(sources, bapi_ui_component_get_parent(comp))) continue;
		bapi_ui_component_t parent = bapi_ui_component_get_parent(comp);
		EditorCloneComponent(state, comp, parent);
	}
}

void EditorCopySelection(EditorState &state)
{
	state.clipboard.clear();
	if (state.selection_list.empty()) return;
	std::vector<bapi_ui_component_t> sources = state.selection_list;
	for (bapi_ui_component_t comp : sources) {
		if (comp_in_list(sources, bapi_ui_component_get_parent(comp))) continue;
		auto owner = std::make_shared<ComponentOwner>();
		owner->comp = bapi_ui_component_clone(comp);
		if (owner->comp) state.clipboard.push_back(owner);
	}
}

void EditorPasteClipboard(EditorState &state)
{
	if (state.clipboard.empty() || !state.ui) return;
	bapi_ui_component_t parent = nullptr;
	if (state.selection && EditorIsContainerType(bapi_ui_component_get_type(state.selection)))
		parent = state.selection;
	for (const auto &master : state.clipboard) {
		if (!master->comp) continue;
		bapi_ui_component_t copy = bapi_ui_component_clone(master->comp);
		if (!copy) continue;
		const char *base = bapi_ui_component_get_id(copy);
		std::string new_id = unique_id(state, base ? base : "component");
		if (bapi_ui_component_set_id(copy, new_id.c_str()) != 0) {
			bapi_ui_component_destroy(copy);
			continue;
		}
		auto owner = std::make_shared<ComponentOwner>();
		owner->comp = copy;
		EditorPushCommand(state, std::make_unique<CloneCmd>(state.ui, owner, parent));
	}
}

void EditorAlignSelection(EditorState &state, const char *align)
{
	if (state.selection_list.size() < 2 || !state.selection) return;
	bapi_ui_component_t ref = state.selection;
	bapi_rect_t			ref_rect;
	bapi_ui_component_get_rect(ref, &ref_rect);
	for (bapi_ui_component_t comp : state.selection_list) {
		if (!comp || comp == ref) continue;
		bapi_rect_t rect;
		bapi_ui_component_get_rect(comp, &rect);
		bapi_rect_t old_rect = rect;
		if (strcmp(align, "left") == 0) {
			rect.x = ref_rect.x;
		} else if (strcmp(align, "center") == 0) {
			rect.x = ref_rect.x + (ref_rect.w - rect.w) * 0.5f;
		} else if (strcmp(align, "right") == 0) {
			rect.x = ref_rect.x + ref_rect.w - rect.w;
		} else if (strcmp(align, "top") == 0) {
			rect.y = ref_rect.y;
		} else if (strcmp(align, "middle") == 0) {
			rect.y = ref_rect.y + (ref_rect.h - rect.h) * 0.5f;
		} else if (strcmp(align, "bottom") == 0) {
			rect.y = ref_rect.y + ref_rect.h - rect.h;
		} else {
			continue;
		}
		if (rect.x != old_rect.x || rect.y != old_rect.y)
			EditorPushCommand(state, std::make_unique<SetRectCmd>(comp, old_rect, rect));
	}
}

void EditorReorderComponent(EditorState &state, bapi_ui_component_t comp, ReorderOp op)
{
	if (!comp || !state.ui) return;
	EditorPushCommand(state, std::make_unique<ReorderCmd>(state.ui, comp, op));
}

// ---------------------------------------------------------------------------
// Undo / redo
// ---------------------------------------------------------------------------

void EditorPushCommand(EditorState &state, std::unique_ptr<Command> cmd)
{
	if (!cmd) return;
	cmd->Execute(state);
	state.undo_stack.push_back(std::move(cmd));
	state.redo_stack.clear();
	EditorMarkDirty(state);
}

bool EditorCanUndo(const EditorState &state)
{
	return !state.undo_stack.empty();
}

bool EditorCanRedo(const EditorState &state)
{
	return !state.redo_stack.empty();
}

bool EditorUndo(EditorState &state)
{
	if (state.undo_stack.empty()) return false;
	std::unique_ptr<Command> cmd = std::move(state.undo_stack.back());
	state.undo_stack.pop_back();
	cmd->Undo(state);
	state.redo_stack.push_back(std::move(cmd));
	EditorMarkDirty(state);
	return true;
}

bool EditorRedo(EditorState &state)
{
	if (state.redo_stack.empty()) return false;
	std::unique_ptr<Command> cmd = std::move(state.redo_stack.back());
	state.redo_stack.pop_back();
	cmd->Execute(state);
	state.undo_stack.push_back(std::move(cmd));
	EditorMarkDirty(state);
	return true;
}

// ---------------------------------------------------------------------------
// Command helpers used by panels
// ---------------------------------------------------------------------------

void EditorAddComponent(EditorState &state, bapi_ui_component_type_t type, const char *id,
						bapi_ui_component_t parent)
{
	EditorPushCommand(state,
					  std::make_unique<AddComponentCmd>(state.ui, type, id, parent));
}

void EditorCreateComponentAt(EditorState &state, bapi_ui_component_type_t type, float doc_x,
							 float doc_y)
{
	if (!state.ui) return;
	const char *name = EditorComponentTypeName(type);
	char		id[128];
	int			suffix = 1;
	for (;;) {
		snprintf(id, sizeof(id), "%s_%d", name, suffix);
		if (!bapi_ui_find(state.ui, id)) break;
		suffix++;
	}
	bapi_ui_component_t parent =
		state.selection && EditorIsContainerType(bapi_ui_component_get_type(state.selection))
			? state.selection
			: nullptr;
	EditorAddComponent(state, type, id, parent);
	bapi_ui_component_t comp = bapi_ui_find(state.ui, id);
	if (comp) {
		EditorSelectComponent(state, comp);
		if (!parent) {
			bapi_rect_t rect;
			bapi_ui_component_get_rect(comp, &rect);
			rect.x = doc_x;
			rect.y = doc_y;
			EditorSetComponentRect(state, comp, rect);
		}
	}
}

void EditorRemoveComponent(EditorState &state, bapi_ui_component_t comp)
{
	if (!comp) return;
	if (state.selection == comp) {
		state.selection = nullptr;
		state.drag_component = nullptr;
		state.dragging		 = false;
	}
	auto it = std::find(state.selection_list.begin(), state.selection_list.end(), comp);
	if (it != state.selection_list.end()) state.selection_list.erase(it);
	EditorPushCommand(state, std::make_unique<RemoveComponentCmd>(state.ui, comp, state.owner_registry));
}

void EditorRemoveComponents(EditorState &state, const std::vector<bapi_ui_component_t> &comps)
{
	std::vector<bapi_ui_component_t> to_remove;
	for (bapi_ui_component_t comp : comps)
		if (comp) to_remove.push_back(comp);
	if (to_remove.empty()) return;
	// drop any component whose ancestor is also being removed
	std::vector<bapi_ui_component_t> top;
	for (bapi_ui_component_t comp : to_remove) {
		bool has_ancestor = false;
		for (bapi_ui_component_t c = bapi_ui_component_get_parent(comp); c; c = bapi_ui_component_get_parent(c)) {
			if (comp_in_list(to_remove, c)) {
				has_ancestor = true;
				break;
			}
		}
		if (!has_ancestor) top.push_back(comp);
	}
	state.selection = nullptr;
	state.selection_list.clear();
	state.drag_component = nullptr;
	state.dragging		 = false;
	EditorPushCommand(state, std::make_unique<MultiRemoveCmd>(state.ui, top, state.owner_registry));
}

void EditorCommitMultiMove(EditorState &state, const std::vector<RectMove> &moves)
{
	std::vector<RectMove> changed;
	for (const RectMove &move : moves) {
		if (move.comp && (move.old_rect.x != move.new_rect.x || move.old_rect.y != move.new_rect.y ||
						  move.old_rect.w != move.new_rect.w || move.old_rect.h != move.new_rect.h))
			changed.push_back(move);
	}
	if (changed.empty()) return;
	EditorPushCommand(state, std::make_unique<MultiRectCmd>(std::move(changed)));
}

void EditorSetComponentRect(EditorState &state, bapi_ui_component_t comp, bapi_rect_t new_rect)
{
	if (!comp) return;
	bapi_rect_t old_rect;
	bapi_ui_component_get_rect(comp, &old_rect);
	EditorPushCommand(state, std::make_unique<SetRectCmd>(comp, old_rect, new_rect));
}

void EditorSetComponentText(EditorState &state, bapi_ui_component_t comp, const char *new_text)
{
	if (!comp) return;
	const char *old_text = bapi_ui_component_get_text(comp);
	EditorPushCommand(state, std::make_unique<SetTextCmd>(comp, old_text, new_text));
}

void EditorSetComponentId(EditorState &state, bapi_ui_component_t comp, const char *new_id)
{
	if (!comp || !new_id || !new_id[0]) return;
	const char *old_id = bapi_ui_component_get_id(comp);
	if (!old_id || strcmp(old_id, new_id) == 0) return;
	EditorPushCommand(state, std::make_unique<SetIdCmd>(comp, old_id, new_id));
}

bool EditorIdIsUnique(EditorState &state, bapi_ui_component_t comp, const char *new_id)
{
	if (!comp || !new_id || !new_id[0]) return false;
	bapi_ui_t ui = state.ui;
	bapi_ui_component_t parent = bapi_ui_component_get_parent(comp);
	int count;
	if (parent) {
		count = bapi_ui_component_get_child_count(parent);
		for (int i = 0; i < count; i++) {
			bapi_ui_component_t sibling = bapi_ui_component_get_child(parent, i);
			if (sibling == comp) continue;
			const char *sid = bapi_ui_component_get_id(sibling);
			if (sid && strcmp(sid, new_id) == 0) return false;
		}
	} else {
		count = bapi_ui_get_root_count(ui);
		for (int i = 0; i < count; i++) {
			bapi_ui_component_t sibling = bapi_ui_get_root(ui, i);
			if (sibling == comp) continue;
			const char *sid = bapi_ui_component_get_id(sibling);
			if (sid && strcmp(sid, new_id) == 0) return false;
		}
	}
	return true;
}

void EditorSetComponentColor(EditorState &state, bapi_ui_component_t comp, bapi_ui_color_role_t role,
							 bapi_color_t new_color)
{
	if (!comp) return;
	bapi_color_t old_color;
	if (bapi_ui_component_get_color(comp, role, &old_color) != 0) return;
	EditorPushCommand(state, std::make_unique<SetColorCmd>(comp, role, old_color, new_color));
}

void EditorSetComponentTextSize(EditorState &state, bapi_ui_component_t comp, float new_size)
{
	if (!comp) return;
	float old_size = bapi_ui_component_get_text_size(comp);
	if (old_size == new_size) return;
	EditorPushCommand(state, std::make_unique<SetTextSizeCmd>(comp, old_size, new_size));
}

void EditorSetComponentValue(EditorState &state, bapi_ui_component_t comp, float new_value)
{
	if (!comp) return;
	float old_value = bapi_ui_component_get_value(comp);
	if (old_value == new_value) return;
	EditorPushCommand(state, std::make_unique<SetValueCmd>(comp, old_value, new_value));
}

void EditorSetComponentColumns(EditorState &state, bapi_ui_component_t comp, int new_value)
{
	if (!comp) return;
	int old_value = bapi_ui_component_get_columns(comp);
	if (old_value == new_value) return;
	EditorPushCommand(state, std::make_unique<SetColumnsCmd>(comp, old_value, new_value));
}

void EditorSetComponentSides(EditorState &state, bapi_ui_component_t comp, int new_value)
{
	if (!comp) return;
	int old_value = bapi_ui_component_get_sides(comp);
	if (old_value == new_value) return;
	EditorPushCommand(state, std::make_unique<SetSidesCmd>(comp, old_value, new_value));
}

void EditorCommitMove(EditorState &state, bapi_ui_component_t comp, bapi_rect_t old_rect,
					  bapi_rect_t new_rect)
{
	if (!comp) return;
	EditorPushCommand(state, std::make_unique<SetRectCmd>(comp, old_rect, new_rect));
}

void EditorSetComponentMin(EditorState &state, bapi_ui_component_t comp, float new_value)
{
	if (!comp) return;
	float old_value = bapi_ui_component_get_min_value(comp);
	if (old_value == new_value) return;
	EditorPushCommand(state, std::make_unique<SetNumericCmd>(comp, NumericField::MinValue, old_value, new_value));
}

void EditorSetComponentMax(EditorState &state, bapi_ui_component_t comp, float new_value)
{
	if (!comp) return;
	float old_value = bapi_ui_component_get_max_value(comp);
	if (old_value == new_value) return;
	EditorPushCommand(state, std::make_unique<SetNumericCmd>(comp, NumericField::MaxValue, old_value, new_value));
}

void EditorSetComponentStep(EditorState &state, bapi_ui_component_t comp, float new_value)
{
	if (!comp) return;
	float old_value = bapi_ui_component_get_step(comp);
	if (old_value == new_value) return;
	EditorPushCommand(state, std::make_unique<SetNumericCmd>(comp, NumericField::Step, old_value, new_value));
}

void EditorSetComponentRadius(EditorState &state, bapi_ui_component_t comp, float new_value)
{
	if (!comp) return;
	float old_value = bapi_ui_component_get_radius(comp);
	if (old_value == new_value) return;
	EditorPushCommand(state, std::make_unique<SetNumericCmd>(comp, NumericField::Radius, old_value, new_value));
}

void EditorSetComponentChecked(EditorState &state, bapi_ui_component_t comp, bool new_value)
{
	if (!comp) return;
	bool old_value = bapi_ui_component_is_checked(comp) != 0;
	if (old_value == new_value) return;
	EditorPushCommand(state, std::make_unique<SetBoolCmd>(comp, BoolField::Checked, old_value, new_value));
}

void EditorSetComponentRelative(EditorState &state, bapi_ui_component_t comp, bool new_value)
{
	if (!comp) return;
	bool old_value = bapi_ui_component_get_relative(comp) != 0;
	if (old_value == new_value) return;
	EditorPushCommand(state, std::make_unique<SetBoolCmd>(comp, BoolField::Relative, old_value, new_value));
}

void EditorSetComponentVisible(EditorState &state, bapi_ui_component_t comp, bool new_value)
{
	if (!comp) return;
	bool old_value = bapi_ui_component_is_visible(comp) != 0;
	if (old_value == new_value) return;
	EditorPushCommand(state, std::make_unique<SetBoolCmd>(comp, BoolField::Visible, old_value, new_value));
}

void EditorSetComponentEnabled(EditorState &state, bapi_ui_component_t comp, bool new_value)
{
	if (!comp) return;
	bool old_value = bapi_ui_component_is_enabled(comp) != 0;
	if (old_value == new_value) return;
	EditorPushCommand(state, std::make_unique<SetBoolCmd>(comp, BoolField::Enabled, old_value, new_value));
}

// ---------------------------------------------------------------------------
// src (resource) editing
// ---------------------------------------------------------------------------

class SetSrcCmd : public Command {
public:
	SetSrcCmd(bapi_ui_component_t comp, const char *old_src, const char *new_src)
		: comp_(comp), old_src_(old_src ? old_src : ""), new_src_(new_src ? new_src : "")
	{
	}

	void Execute(EditorState &state) override
	{
		(void)state;
		if (!new_src_.empty()) bapi_ui_component_set_src(comp_, new_src_.c_str());
	}

	void Undo(EditorState &state) override
	{
		(void)state;
		if (!old_src_.empty()) bapi_ui_component_set_src(comp_, old_src_.c_str());
	}

	const char *Name() const override { return "Change Resource"; }

private:
	bapi_ui_component_t comp_;
	std::string			old_src_;
	std::string			new_src_;
};

void EditorSetComponentSrc(EditorState &state, bapi_ui_component_t comp, const char *new_src)
{
	if (!comp || !new_src || !new_src[0]) return;
	const char *old_src = bapi_ui_component_get_src(comp);
	if (old_src && strcmp(old_src, new_src) == 0) return;
	// load the new resource first; only record the command if it succeeded
	if (bapi_ui_component_set_src(comp, new_src) != 0) return;
	EditorPushCommand(state, std::make_unique<SetSrcCmd>(comp, old_src, new_src));
}

// ---------------------------------------------------------------------------
// scene switching (M4): one undo step that flips visibility of every root that
// is not marked persistent, leaving the active scene visible and the others
// hidden. Persistent roots are skipped (their visibility is untouched).
// ---------------------------------------------------------------------------

class SceneSwitchCmd : public Command {
public:
	SceneSwitchCmd(bapi_ui_component_t target) : target_(target) {}

	void Execute(EditorState &state) override
	{
		if (!state.ui || !target_) return;
		if (!collected_) {
			collected_		= true;
			old_active_		= state.active_scene;
			int root_count	= bapi_ui_get_root_count(state.ui);
			for (int i = 0; i < root_count; i++) {
				bapi_ui_component_t root = bapi_ui_get_root(state.ui, i);
				if (!root) continue;
				if (state.persistent_roots.count(root)) continue;
				old_visible_.push_back({root, bapi_ui_component_is_visible(root) != 0});
			}
		}
		state.active_scene = target_;
		for (auto &entry : old_visible_) {
			bapi_ui_component_set_visible(entry.first, entry.first == target_ ? 1 : 0);
		}
	}

	void Undo(EditorState &state) override
	{
		state.active_scene = old_active_;
		for (auto &entry : old_visible_) {
			bapi_ui_component_set_visible(entry.first, entry.second ? 1 : 0);
		}
	}

	const char *Name() const override { return "Switch Scene"; }

private:
	bapi_ui_component_t target_ = nullptr;
	bapi_ui_component_t old_active_ = nullptr;
	std::vector<std::pair<bapi_ui_component_t, bool>> old_visible_;
	bool collected_ = false;
};

void EditorSwitchScene(EditorState &state, bapi_ui_component_t scene_root)
{
	if (!state.ui || !scene_root) return;
	if (state.persistent_roots.count(scene_root)) return;
	if (state.active_scene == scene_root) return;
	EditorPushCommand(state, std::make_unique<SceneSwitchCmd>(scene_root));
}

void EditorToggleScenePersistent(EditorState &state, bapi_ui_component_t root)
{
	if (!state.ui || !root) return;
	if (state.persistent_roots.count(root)) {
		state.persistent_roots.erase(root);
	} else {
		state.persistent_roots.insert(root);
		// a persistent root is never a switchable scene; drop it if it was
		if (state.active_scene == root) state.active_scene = nullptr;
	}
}
