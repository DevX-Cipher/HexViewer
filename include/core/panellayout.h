#pragma once
#include "render.h"
#include "panelcontent.h"

struct Bookmark;

enum class UIItemType {
    SectionHeader,
    LabelValue,
    Text,
    Bookmark,
    Separator,
    Spacer
};

struct UIItem {
    UIItemType type;
    Rect bounds;

    const char* label;
    const char* value;

    Color labelColor;
    Color valueColor;
    Color color;

    int dataIndex;
    bool clickable;
    bool selected;

    UIItem();
};

class PanelLayoutBuilder {
private:
    Vector<UIItem> items;

    int x;
    int y;
    int width;

    int rowHeight;
    int headerHeight;
    int itemSpacing;
    int sectionSpacing;

    int (*measureTextHeight)(const char*);

public:
    PanelLayoutBuilder(
        int startX,
        int startY,
        int panelWidth,
        int rowH,
        int headerH,
        int itemSpace,
        int sectionSpace,
        int (*measureFn)(const char*)
    );

    void addSectionHeader(const char* text, const Color& color);
    void addLabelValue(const char* label, const char* value,
                       const Color& labelColor, const Color& valueColor);
    void addText(const char* text, const Color& color, bool clickable = false);
    void addBookmark(const Bookmark& bm, int index, bool selected);
    void addSeparator(const Color& color);
    void addSpacer(int pixels);

    const Vector<UIItem>& getItems() const;
    int getFinalHeight() const;

    int hitTest(int mx, int my) const;
};
