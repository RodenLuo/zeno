#include <cmath>
#include <cstdio>
#include <thread>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <variant>
#include <optional>
#include <functional>
#if defined(__linux__)
#include <unistd.h>
#endif
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <FTGL/ftgl.h>
#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <set>


// BEG generic ui library

struct Color {
    float r, g, b;

    Color(float r = 0, float g = 0, float b = 0)
        : r(r), g(g), b(b) {}

    float *data() {
        return &r;
    }

    float const *data() const {
        return &r;
    }
};

struct Point {
    float x, y;

    Point(float x = 0, float y = 0)
        : x(x), y(y) {}

    Point operator+(Point const &o) const {
        return {x + o.x, y + o.y};
    }

    Point operator-(Point const &o) const {
        return {x - o.x, y - o.y};
    }
};

struct AABB {
    float x0, y0, nx, ny;

    AABB(float x0 = 0, float y0 = 0, float nx = 0, float ny = 0)
        : x0(x0), y0(y0), nx(nx), ny(ny) {}

    bool contains(float x, float y) const {
        return x0 <= x && y0 <= y && x <= x0 + nx && y <= y0 + ny;
    }
};


struct Font {
    std::unique_ptr<FTFont> font;
    std::unique_ptr<FTSimpleLayout> layout;
    float fixed_height = -1;

    Font(const char *path) {
        font = std::make_unique<FTTextureFont>(path);
        if (font->Error()) {
            fprintf(stderr, "Failed to load font: %s\n", path);
            abort();
        }
        font->CharMap(ft_encoding_unicode);

        layout = std::make_unique<FTSimpleLayout>();
        layout->SetFont(font.get());
    }

    Font &set_font_size(float font_size) {
        font->FaceSize(font_size);
        return *this;
    }

    Font &set_fixed_width(float width, FTGL::TextAlignment align = FTGL::ALIGN_CENTER) {
        layout->SetLineLength(width);
        layout->SetAlignment(align);
        return *this;
    }

    Font &set_fixed_height(float height) {
        fixed_height = height;
        return *this;
    }

    AABB calc_bounding_box(std::string const &str) {
        auto bbox = layout->BBox(str.data(), str.size());
        return AABB(bbox.Lower().X(), bbox.Lower().Y(),
                    bbox.Upper().X() - bbox.Lower().X(),
                    bbox.Upper().Y() - bbox.Lower().Y());
    }

    Font &render(float x, float y, std::string const &str) {
        if (fixed_height > 0) {
            auto bbox = calc_bounding_box(str);
            y += fixed_height / 2 - bbox.ny / 2;
        }
        if (str.size()) {
            glPushMatrix();
            glTranslatef(x, y, 0.f);
            layout->Render(str.data(), str.size());
            glPopMatrix();
        }
        return *this;
    }
};


GLFWwindow *window;


struct Widget;

struct CursorState {
    float x = 0, y = 0;
    float dx = 0, dy = 0;
    float last_x = 0, last_y = 0;
    bool lmb = false, mmb = false, rmb = false;
    bool last_lmb = false, last_mmb = false, last_rmb = false;
    bool shift = false, ctrl = false, alt = false;
    bool del = false, last_del = false;

    void on_update() {
        last_lmb = lmb;
        last_mmb = mmb;
        last_rmb = rmb;
        last_del = del;
        last_x = x;
        last_y = y;

        GLint nx, ny;
        glfwGetFramebufferSize(window, &nx, &ny);
        GLdouble _x, _y;
        glfwGetCursorPos(window, &_x, &_y);
        x = 0.5f + (float)_x;
        y = ny - 0.5f - (float)_y;
        dx = x - last_x;
        dy = y - last_y;
        lmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        mmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
        rmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
        ctrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
        alt = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;
        del = glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS;
    }

    bool is_pressed(int key) const {
        return glfwGetKey(window, key) == GLFW_PRESS;
    }

    auto translate(float dx, float dy) {
        auto ox = x, oy = y;
        x += dx; y += dy;
        struct RAII : std::function<void()> {
            using std::function<void()>::function;
            ~RAII() { (*this)(); }
        } raii {[=] () {
            x = ox; y = oy;
        }};
        return raii;
    }
} cur;


template <class T>
inline T notnull(T &&t) {
    if (!t) throw std::bad_optional_access();
    return t;
}

struct Object {
    Object() = default;
    Object(Object const &) = delete;
    Object &operator=(Object const &) = delete;
    Object(Object &&) = delete;
    Object &operator=(Object &&) = delete;
    virtual ~Object() = default;
};

/*template <class T>
struct Ptr : std::unique_ptr<T> {
    using std::unique_ptr<T>::unique_ptr;

    Ptr(std::unique_ptr<T> &&p) : std::unique_ptr<T>(std::move(p)) {}
    Ptr(std::unique_ptr<T> const &p) : std::unique_ptr<T>(p) {}
    Ptr(T *p) : std::unique_ptr<T>(p) {}
    operator T *() const { return std::unique_ptr<T>::get(); }
};

template <class T>
Ptr(T *) -> Ptr<T>;

template <class T>
Ptr(std::unique_ptr<T>) -> Ptr<T>;

template <class T, class ...Ts>
Ptr<T> makePtr(Ts &&...ts) {
    return std::make_unique<T>(std::forward<Ts>(ts)...);
}*/


struct Widget : Object {
    Widget *parent = nullptr;
    std::vector<std::unique_ptr<Widget>> children;
    Point position{0, 0};
    float zvalue{0};

    template <class T, class ...Ts>
    T *add_child(Ts &&...ts) {
        auto p = std::make_unique<T>(std::forward<Ts>(ts)...);
        T *raw_p = p.get();
        p->parent = this;
        children.push_back(std::move(p));
        return raw_p;
    }

    bool remove_child(Widget *ptr) {
        for (auto &child: children) {
            if (child.get() == ptr) {
                ptr->parent = nullptr;
                child = nullptr;
                return true;
            }
        }
        return false;
    }

    virtual AABB get_bounding_box() const = 0;

    virtual void on_hover_enter() {
    }

    virtual void on_hover_leave() {
        if (lmb_pressed) on_lmb_up();
        if (mmb_pressed) on_mmb_up();
        if (rmb_pressed) on_rmb_up();
    }

    bool hovered = false;

    virtual void on_mouse_move() {
    }

    virtual void on_lmb_down() {
        lmb_pressed = true;
    }

    virtual void on_lmb_up() {
        lmb_pressed = false;
    }

    virtual void on_mmb_down() {
        mmb_pressed = true;
    }

    virtual void on_mmb_up() {
        mmb_pressed = false;
    }

    virtual void on_rmb_down() {
        rmb_pressed = true;
    }

    virtual void on_rmb_up() {
        rmb_pressed = false;
    }

    virtual void on_del_down() {
    }

    virtual void on_del_up() {
    }

    bool lmb_pressed = false;
    bool mmb_pressed = false;
    bool rmb_pressed = false;

    void _clean_null_children() {
        bool has_any;
        do {
            has_any = false;
            for (auto it = children.begin(); it != children.end(); it++) {
                if (!*it) {
                    children.erase(it);
                    has_any = true;
                    break;
                }
            }
        } while (has_any);
    }

    virtual Widget *item_at(Point p) const {
        auto bbox = get_bounding_box();
        if (!bbox.contains(p.x, p.y)) {
            return nullptr;
        }
        Widget *found = nullptr;
        for (auto const &child: children) {
            if (child) {
                if (auto it = child->item_at(p - child->position); it) {
                    if (!found || it->zvalue >= found->zvalue) {
                        found = it;
                    }
                }
            }
        }
        if (found)
            return found;
        return const_cast<Widget *>(this);
    }

    virtual void do_update() {
        auto raii = cur.translate(-position.x, -position.y);
        auto bbox = get_bounding_box();

        auto old_hovered = hovered;
        hovered = bbox.contains(cur.x, cur.y);

        for (auto const &child: children) {
            if (child)
                child->do_update();
        }

        if (hovered) {
            if (!cur.last_del && cur.del) {
                on_del_down();
            }
            if (!cur.last_lmb && cur.lmb) {
                on_lmb_down();
                //cur.last_lmb = cur.lmb;
            }
            if (!cur.last_mmb && cur.mmb) {
                on_mmb_down();
                //cur.last_mmb = cur.mmb;
            }
            if (!cur.last_rmb && cur.rmb) {
                on_rmb_down();
                //cur.last_rmb = cur.rmb;
            }
        }

        if (cur.dx || cur.dy) {
            on_mouse_move();
        }

        if (hovered) {
            if (cur.last_lmb && !cur.lmb) {
                on_lmb_up();
                //cur.last_lmb = cur.lmb;
            }
            if (cur.last_mmb && !cur.mmb) {
                on_mmb_up();
                //cur.last_mmb = cur.mmb;
            }
            if (cur.last_rmb && !cur.rmb) {
                on_rmb_up();
                //cur.last_rmb = cur.rmb;
            }
            if (cur.last_del && !cur.del) {
                on_del_up();
            }
        }

        if (!old_hovered && hovered) {
            on_hover_enter();
        } else if (old_hovered && !hovered) {
            on_hover_leave();
        }
    }

    virtual void do_paint() {
        auto raii = cur.translate(-position.x, -position.y);
        glPushMatrix();
        glTranslatef(position.x, position.y, zvalue);
        paint();
        for (auto const &child: children) {
            if (child)
                child->do_paint();
        }
        glPopMatrix();
    }

    virtual void paint() const {}
};


struct GraphicsWidget : Widget {
    bool selected = false;
    bool selectable = false;
    bool draggable = false;
};


struct GraphicsView : Widget {
    std::set<GraphicsWidget *> children_selected;

    void _deselect_children() {
        for (auto const &child: children_selected) {
            child->selected = false;
        }
        children_selected.clear();
    }

    void _select_child(GraphicsWidget *ptr, bool multiselect = false) {
        if (!(multiselect || ptr->selected)) {
            _deselect_children();
        }
        if (ptr) {
            if (ptr->selected && multiselect) {
                children_selected.erase(ptr);
                ptr->selected = false;
            } else {
                children_selected.insert(ptr);
                ptr->selected = true;
            }
        }
    }

    void on_mouse_move() override {
        Widget::on_mouse_move();
        if (cur.lmb) {
            for (auto const &child: children_selected) {
                if (child->draggable) {
                    child->position.x += cur.dx;
                    child->position.y += cur.dy;
                }
            }
        }
    }

    void on_lmb_down() override {
        Widget::on_lmb_down();
        if (auto item = dynamic_cast<GraphicsWidget *>(item_at({cur.x, cur.y})); item) {
            if (item->selectable)
                _select_child(item, cur.shift);
        } else if (!cur.shift) {
            _deselect_children();
        }
    }
};


struct GraphicsRectItem : GraphicsWidget {
    AABB bbox{0, 0, 200, 150};

    void set_bounding_box(AABB bbox) {
        this->bbox = bbox;
    }

    AABB get_bounding_box() const override {
        return bbox;
    }

    void paint() const override {
        if (selected || lmb_pressed) {
            glColor3f(0.75f, 0.5f, 0.375f);
        } else if (hovered) {
            glColor3f(0.375f, 0.5f, 1.0f);
        } else {
            glColor3f(0.375f, 0.375f, 0.375f);
        }
        glRectf(bbox.x0, bbox.y0, bbox.x0 + bbox.nx, bbox.y0 + bbox.ny);
    }
};


struct Button : Widget {
    AABB bbox{0, 0, 150, 50};
    std::string text;

    void set_bounding_box(AABB bbox) {
        this->bbox = bbox;
    }

    AABB get_bounding_box() const override {
        return bbox;
    }

    virtual void on_clicked() {}

    void on_lmb_down() override {
        Widget::on_lmb_down();
        on_clicked();
    }

    void paint() const override {
        if (lmb_pressed) {
            glColor3f(0.75f, 0.5f, 0.375f);
        } else if (hovered) {
            glColor3f(0.375f, 0.5f, 1.0f);
        } else {
            glColor3f(0.375f, 0.375f, 0.375f);
        }
        glRectf(bbox.x0, bbox.y0, bbox.x0 + bbox.nx, bbox.y0 + bbox.ny);

        if (text.size()) {
            Font font("LiberationMono-Regular.ttf");
            //Font font("/usr/share/fonts/wenquanyi/wqy-microhei/wqy-microhei.ttc");
            font.set_font_size(30.f);
            font.set_fixed_width(bbox.nx);
            font.set_fixed_height(bbox.ny);
            glColor3f(1.f, 1.f, 1.f);
            font.render(bbox.x0, bbox.y0, text);
        }
    }
};

// END generic ui library

// BEG node data structures

struct DopInputValue_NoValue {

    void serialize(std::ostream &ss) const {
        ss << "none";
    }
};

struct DopInputValue_ReferVariable {
    std::string refid;

    void serialize(std::ostream &ss) const {
        ss << "ref " << refid;
    }
};

struct DopInputValue_ImmedValue {
    std::string immed;

    void serialize(std::ostream &ss) const {
        ss << "imm " << immed;
    }
};

using DopInputValue = std::variant
    < DopInputValue_NoValue
    , DopInputValue_ReferVariable
    , DopInputValue_ImmedValue
    >;

struct DopInputSocket {
    std::string name;
    DopInputValue value;

    void serialize(std::ostream &ss) const {
        ss << name << "=";
        std::visit([&] (auto &value) {
            value.serialize(ss);
        }, value);
    }
};

struct DopOutputSocket {
    std::string name;

    void serialize(std::ostream &ss) const {
        ss << name;
    }
};

struct DopNode {
    std::string name;
    std::string kind;
    std::vector<DopInputSocket> inputs;
    std::vector<DopOutputSocket> outputs;

    void serialize(std::ostream &ss) const {
        ss << "DopNode[" << '\n';
        ss << "  name=" << name << '\n';
        ss << "  kind=" << kind << '\n';
        ss << "  inputs=[" << '\n';
        for (auto const &input: inputs) {
            ss << "    ";
            input.serialize(ss);
            ss << '\n';
        }
        ss << "  ]" << '\n';
        ss << "  outputs=[" << '\n';
        for (auto const &output: outputs) {
            ss << "    ";
            output.serialize(ss);
            ss << '\n';
        }
        ss << "  ]" << '\n';
        ss << "]" << '\n';
    }
};


struct DopGraph {
    std::set<std::unique_ptr<DopNode>> nodes;

    DopNode *add_node() {
        auto p = std::make_unique<DopNode>();
        auto raw = p.get();
        nodes.insert(std::move(p));
        return raw;
    }

    void serialize(std::ostream &ss) const {
        for (auto const &node: nodes) {
            node->serialize(ss);
            ss << '\n';
        }
    }

    static void set_node_input
        ( DopNode *to_node
        , int to_socket_index
        , DopNode *from_node
        , int from_socket_index
        )
    {
        auto const &from_socket = from_node->outputs.at(from_socket_index);
        auto &to_socket = to_node->inputs.at(to_socket_index);
        auto refid = from_node->name + ":" + from_socket.name;
        to_socket.value = DopInputValue_ReferVariable{refid};
    }

    static void remove_node_input
        ( DopNode *to_node
        , int to_socket_index
        )
    {
        auto &to_socket = to_node->inputs.at(to_socket_index);
        to_socket.value = DopInputValue_NoValue{};
    }
};

// END node data structures

// BEG node editor ui

struct UiDopLink;
struct UiDopNode;

struct UiDopSocket : GraphicsRectItem {
    static constexpr float BW = 4, R = 15, FH = 19, NW = 200;

    std::string name;
    std::set<UiDopLink *> links;

    UiDopSocket() {
        set_bounding_box({-R, -R, 2 * R, 2 * R});
        zvalue = 2.f;
    }

    void paint() const override {
        glColor3f(0.75f, 0.75f, 0.75f);
        glRectf(bbox.x0, bbox.y0, bbox.x0 + bbox.nx, bbox.y0 + bbox.ny);
        if (hovered) {
            glColor3f(0.75f, 0.5f, 0.375f);
        } else if (links.size()) {
            glColor3f(0.375f, 0.5f, 1.0f);
        } else {
            glColor3f(0.375f, 0.375f, 0.375f);
        }
        glRectf(bbox.x0 + BW, bbox.y0 + BW, bbox.x0 + bbox.nx - BW, bbox.y0 + bbox.ny - BW);
    }

    UiDopNode *get_parent() const {
        return (UiDopNode *)(parent);
    }

    void clear_links();
};


struct UiDopInputSocket : UiDopSocket {
    void paint() const override {
        UiDopSocket::paint();

        if (hovered) {
            Font font("LiberationMono-Regular.ttf");
            font.set_font_size(FH);
            font.set_fixed_height(2 * R);
            font.set_fixed_width(NW, FTGL::ALIGN_LEFT);
            glColor3f(1.f, 1.f, 1.f);
            font.render(R * 1.3f, -R + FH * 0.15f, name);
        }
    }

    void attach_link(UiDopLink *link) {
        clear_links();
        links.insert(link);
    }

    int get_index() const;
};


struct UiDopOutputSocket : UiDopSocket {
    void paint() const override {
        UiDopSocket::paint();

        if (hovered) {
            Font font("LiberationMono-Regular.ttf");
            font.set_font_size(FH);
            font.set_fixed_height(2 * R);
            font.set_fixed_width(NW, FTGL::ALIGN_RIGHT);
            glColor3f(1.f, 1.f, 1.f);
            font.render(-NW - R * 1.5f, -R + FH * 0.15f, name);
        }
    }

    void attach_link(UiDopLink *link) {
        links.insert(link);
    }

    int get_index() const;
};


struct UiDopGraph;


struct UiDopNode : GraphicsRectItem {
    static constexpr float DH = 40, TH = 42, FH = 24, W = 200, BW = 3;

    std::vector<UiDopInputSocket *> inputs;
    std::vector<UiDopOutputSocket *> outputs;
    std::string name;
    std::string kind;

    DopNode *bk_node = nullptr;

    void _update_backend_data() const {
        bk_node->name = name;
        bk_node->inputs.resize(inputs.size());
        for (int i = 0; i < inputs.size(); i++) {
            bk_node->inputs[i].name = inputs[i]->name;
        }
        bk_node->outputs.resize(outputs.size());
        for (int i = 0; i < outputs.size(); i++) {
            bk_node->outputs[i].name = outputs[i]->name;
        }
    }

    void update_sockets() {
        for (int i = 0; i < inputs.size(); i++) {
            auto y = DH * (i + 0.5f);
            inputs[i]->position = {UiDopSocket::R, -y};
        }
        for (int i = 0; i < outputs.size(); i++) {
            auto y = DH * (i + 0.5f);
            outputs[i]->position = {W - UiDopSocket::R, -y};
        }
        auto h = std::max(outputs.size(), inputs.size()) * DH;
        set_bounding_box({0, -h, W, h + TH});

        _update_backend_data();
    }

    UiDopInputSocket *add_input_socket() {
        auto p = add_child<UiDopInputSocket>();
        inputs.push_back(p);
        return p;
    }

    UiDopOutputSocket *add_output_socket() {
        auto p = add_child<UiDopOutputSocket>();
        outputs.push_back(p);
        return p;
    }

    UiDopNode() {
        selectable = true;
        draggable = true;
        set_bounding_box({0, 0, W, TH});
    }

    UiDopGraph *get_parent() const {
        return (UiDopGraph *)(parent);
    }

    void paint() const override {
        if (selected) {
            glColor3f(0.75f, 0.5f, 0.375f);
        } else {
            glColor3f(0.125f, 0.375f, 0.425f);
        }
        glRectf(bbox.x0 - BW, bbox.y0 - BW, bbox.x0 + bbox.nx + BW, bbox.y0 + bbox.ny + BW);

        glColor3f(0.375f, 0.375f, 0.375f);
        glRectf(bbox.x0, bbox.y0, bbox.x0 + bbox.nx, bbox.y0 + bbox.ny);

        if (selected) {
            glColor3f(0.75f, 0.5f, 0.375f);
        } else {
            glColor3f(0.125f, 0.375f, 0.425f);
        }
        glRectf(0.f, 0.f, W, TH);

        Font font("LiberationMono-Regular.ttf");
        font.set_font_size(FH);
        font.set_fixed_width(W);
        font.set_fixed_height(TH);
        glColor3f(1.f, 1.f, 1.f);
        font.render(0, FH * 0.05f, name);
    }
};

int UiDopInputSocket::get_index() const {
    auto node = get_parent();
    for (int i = 0; i < node->inputs.size(); i++) {
        if (node->inputs[i] == this) {
            return i;
        }
    }
    throw "Cannot find index of input node";
}

int UiDopOutputSocket::get_index() const {
    auto node = get_parent();
    for (int i = 0; i < node->outputs.size(); i++) {
        if (node->outputs[i] == this) {
            return i;
        }
    }
    throw "Cannot find index of output node";
}


struct GraphicsLineItem : GraphicsWidget {
    static constexpr float LW = 5.f;

    virtual Point get_from_position() const = 0;
    virtual Point get_to_position() const = 0;

    AABB get_bounding_box() const override {
        auto [sx, sy] = get_from_position();
        auto [dx, dy] = get_to_position();
        return {std::min(sx, dx) - LW, std::min(sy, dy) - LW, std::fabs(sx - dx) + 2 * LW, std::fabs(sy - dy) + 2 * LW};
    }

    virtual Color get_line_color() const {
        if (selected) {
            return {0.75f, 0.5f, 0.375f};
        } else {
            return {0.375f, 0.5f, 1.0f};
        }
    }

    void paint() const override {
        glColor3fv(get_line_color().data());
        auto [sx, sy] = get_from_position();
        auto [dx, dy] = get_to_position();
        glLineWidth(LW);
        glBegin(GL_LINE_STRIP);
        glVertex2f(sx, sy);
        glVertex2f(dx, dy);
        glEnd();
    }
};


struct UiDopLink : GraphicsLineItem {
    UiDopOutputSocket *from_socket;
    UiDopInputSocket *to_socket;

    UiDopLink(UiDopOutputSocket *from_socket, UiDopInputSocket *to_socket)
        : from_socket(from_socket), to_socket(to_socket)
    {
        from_socket->attach_link(this);
        to_socket->attach_link(this);
        selectable = true;
        zvalue = 1.f;
    }

    Point get_from_position() const override {
        return from_socket->position + from_socket->get_parent()->position;
    }

    Point get_to_position() const override {
        return to_socket->position + to_socket->get_parent()->position;
    }

    UiDopGraph *get_parent() const {
        return (UiDopGraph *)(parent);
    }
};


struct UiDopPendingLink : GraphicsLineItem {
    UiDopSocket *socket;

    UiDopPendingLink(UiDopSocket *socket)
        : socket(socket)
    {
        zvalue = 3.f;
    }

    Color get_line_color() const override {
        return {0.75f, 0.5f, 0.375f};
    }

    Point get_from_position() const override {
        return socket->position + socket->get_parent()->position;
    }

    Point get_to_position() const override {
        return {cur.x, cur.y};
    }

    UiDopGraph *get_parent() const {
        return (UiDopGraph *)(parent);
    }

    Widget *item_at(Point p) const override {
        return nullptr;
    }
};


struct UiDopGraph : GraphicsView {
    std::set<UiDopNode *> nodes;
    std::set<UiDopLink *> links;
    UiDopPendingLink *pending_link = nullptr;
    AABB bbox{0, 0, 400, 400};

    std::unique_ptr<DopGraph> bk_graph = std::make_unique<DopGraph>();

    void set_bounding_box(AABB bbox) {
        this->bbox = bbox;
    }

    AABB get_bounding_box() const override {
        bk_graph->serialize(std::cout);
        return bbox;
    }

    bool remove_link(UiDopLink *link) {
        if (remove_child(link)) {
            link->from_socket->links.erase(link);
            link->to_socket->links.erase(link);
            auto to_node = link->to_socket->get_parent();
            auto from_node = link->from_socket->get_parent();
            bk_graph->remove_node_input(to_node->bk_node,
                    link->to_socket->get_index());
            links.erase(link);
            return true;
        } else {
            return false;
        }
    }

    bool remove_node(UiDopNode *node) {
        for (auto *socket: node->inputs) {
            for (auto *link: std::set(socket->links)) {
                remove_link(link);
            }
        }
        for (auto *socket: node->outputs) {
            for (auto *link: std::set(socket->links)) {
                remove_link(link);
            }
        }
        if (remove_child(node)) {
            nodes.erase(node);
            return true;
        } else {
            return false;
        }
    }

    UiDopNode *add_node() {
        auto p = add_child<UiDopNode>();
        p->bk_node = bk_graph->add_node();
        nodes.insert(p);
        return p;
    }

    UiDopLink *add_link(UiDopOutputSocket *from_socket, UiDopInputSocket *to_socket) {
        auto p = add_child<UiDopLink>(from_socket, to_socket);
        auto to_node = to_socket->get_parent();
        auto from_node = from_socket->get_parent();
        bk_graph->set_node_input(to_node->bk_node, to_socket->get_index(),
                from_node->bk_node, from_socket->get_index());
        links.insert(p);
        return p;
    }

    void add_pending_link(UiDopSocket *socket) {
        if (pending_link) {
            if (socket && pending_link->socket) {
                auto socket1 = pending_link->socket;
                auto socket2 = socket;
                auto output1 = dynamic_cast<UiDopOutputSocket *>(socket1);
                auto output2 = dynamic_cast<UiDopOutputSocket *>(socket2);
                auto input1 = dynamic_cast<UiDopInputSocket *>(socket1);
                auto input2 = dynamic_cast<UiDopInputSocket *>(socket2);
                if (output1 && input2) {
                    add_link(output1, input2);
                } else if (input1 && output2) {
                    add_link(output2, input1);
                }
            }
            remove_child(pending_link);
            pending_link = nullptr;

        } else if (socket) {
            pending_link = add_child<UiDopPendingLink>(socket);
        }
    }

    UiDopGraph() {
        set_bounding_box({0, 0, 550, 400});

        auto c = add_node();
        c->position = {50, 300};
        c->name = "readvdb1";
        c->kind = "readvdb";
        c->add_input_socket()->name = "path";
        c->add_input_socket()->name = "type";
        c->add_output_socket()->name = "grid";
        c->update_sockets();

        auto d = add_node();
        d->position = {300, 300};
        d->name = "vdbsmooth1";
        d->kind = "vdbsmooth";
        d->add_input_socket()->name = "grid";
        d->add_input_socket()->name = "width";
        d->add_input_socket()->name = "times";
        d->add_output_socket()->name = "grid";
        d->update_sockets();

        add_link(c->outputs[0], d->inputs[0]);
    }

    void paint() const override {
        glColor3f(0.2f, 0.2f, 0.2f);
        glRectf(bbox.x0, bbox.y0, bbox.x0 + bbox.nx, bbox.y0 + bbox.ny);
    }

    void on_lmb_down() override {
        GraphicsView::on_lmb_down();

        auto item = item_at({cur.x, cur.y});

        if (auto node = dynamic_cast<UiDopNode *>(item); node) {
            if (pending_link) {
                auto another = pending_link->socket;
                if (dynamic_cast<UiDopInputSocket *>(another) && node->outputs.size()) {
                    add_pending_link(node->outputs[0]);
                } else if (dynamic_cast<UiDopOutputSocket *>(another) && node->inputs.size()) {
                    add_pending_link(node->inputs[0]);
                } else {
                    add_pending_link(nullptr);
                }
            }

        } else if (auto link = dynamic_cast<UiDopLink *>(item); link) {
            if (pending_link) {
                auto another = pending_link->socket;
                if (dynamic_cast<UiDopInputSocket *>(another)) {
                    add_pending_link(link->from_socket);
                } else if (dynamic_cast<UiDopOutputSocket *>(another)) {
                    add_pending_link(link->to_socket);
                } else {
                    add_pending_link(nullptr);
                }
            }

        } else if (auto socket = dynamic_cast<UiDopSocket *>(item); socket) {
            add_pending_link(socket);

        } else {
            if (pending_link) {
                if (auto socket = dynamic_cast<UiDopInputSocket *>(pending_link->socket); socket) {
                    socket->clear_links();
                }
            }
            add_pending_link(nullptr);

        }
    }

    void on_del_down() override {
        Widget::on_del_down();
        for (auto *item: children_selected) {

            if (auto link = dynamic_cast<UiDopLink *>(item); link) {
                remove_link(link);

            } else if (auto node = dynamic_cast<UiDopNode *>(item); node) {
                remove_node(node);
            }
        }
    }
};


void UiDopSocket::clear_links() {
    auto graph = get_parent()->get_parent();
    if (links.size()) {
        for (auto link: std::set(links)) {
            graph->remove_link(link);
        }
    }
}

// END node editor ui

// BEG main window ui

struct RootWindow : Widget {
    RootWindow() {
        add_child<UiDopGraph>();
    }

    AABB get_bounding_box() const override {
        return {0, 0, 800, 600};
    }
} win;

// END main window ui

// BEG ui library main loop

void process_input() {
    GLint nx = 100, ny = 100;
    glfwGetFramebufferSize(window, &nx, &ny);
    glViewport(0, 0, nx, ny);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef(2.f, 2.f, -.001f);
    glTranslatef(-.5f, -.5f, 1.f);
    glScalef(1.f / nx, 1.f / ny, 1.f);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    cur.on_update();
    win.position = {100, 100};
    win.do_update();
}


bool need_repaint = true;

void draw_graphics() {
    if (need_repaint) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        win.do_paint();
        glfwSwapBuffers(window);
    }
}


int main() {
    if (!glfwInit()) {
        const char *err = "unknown error"; glfwGetError(&err);
        fprintf(stderr, "Failed to initialize GLFW library: %s\n", err);
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    window = glfwCreateWindow(800, 600, "Zeno Editor", nullptr, nullptr);
    if (!window) {
        const char *err = "unknown error"; glfwGetError(&err);
        fprintf(stderr, "Failed to create GLFW window: %s\n", err);
        return -1;
    }
    glfwMakeContextCurrent(window);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    double fps = 144;
    double lasttime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        process_input();
        draw_graphics();
        glfwPollEvents();
        if (fps > 0) {
            lasttime += 1.0 / fps;
            while (glfwGetTime() < lasttime) {
                double sleepfor = (lasttime - glfwGetTime()) * 0.75;
                int us(sleepfor / 1000000);
#if defined(__linux__)
                ::usleep(us);
#else
                std::this_thread::sleep_for(std::chrono::microseconds(us));
#endif
            }
        }
    }

    return 0;
}

// END ui library main loop
