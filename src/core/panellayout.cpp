#include "panellayout.h"

UIItem::UIItem()
    : type(UIItemType::Text),
      bounds(),
      label(nullptr),
      value(nullptr),
      labelColor(),
      valueColor(),
      color(),
      dataIndex(-1),
      clickable(false),
      selected(false)
{}

PanelLayoutBuilder::PanelLayoutBuilder(
    int startX,
    int startY,
    int panelWidth,
    int rowH,
    int headerH,
    int itemSpace,
    int sectionSpace,
    int (*measureFn)(const char*)
)
    : x(startX),
      y(startY),
      width(panelWidth),
      rowHeight(rowH),
      headerHeight(headerH),
      itemSpacing(itemSpace),
      sectionSpacing(sectionSpace),
      measureTextHeight(measureFn)
{}

void PanelLayoutBuilder::addSectionHeader(const char* text, const Color& color)
{
    int h = measureTextHeight(text);

    UIItem item;
    item.type = UIItemType::SectionHeader;
    item.label = text;
    item.labelColor = color;
    item.bounds = Rect(x, y, width, h);

    items.push_back(item);
    y += h + sectionSpacing;
}

void PanelLayoutBuilder::addLabelValue(const char* label, const char* value,
                                       const Color& labelColor, const Color& valueColor)
{
    int h = measureTextHeight(label);

    UIItem item;
    item.type = UIItemType::LabelValue;
    item.label = label;
    item.value = value;
    item.labelColor = labelColor;
    item.valueColor = valueColor;
    item.bounds = Rect(x, y, width, h);

    items.push_back(item);
    y += h + itemSpacing;
}

void PanelLayoutBuilder::addText(const char* text, const Color& color, bool clickable)
{
    int h = measureTextHeight(text);

    UIItem item;
    item.type = UIItemType::Text;
    item.label = text;
    item.labelColor = color;
    item.clickable = clickable;
    item.bounds = Rect(x, y, width, h);

    items.push_back(item);
    y += h + itemSpacing;
}

void PanelLayoutBuilder::addBookmark(const Bookmark& bm, int index, bool selected)
{
    int h = rowHeight;

    UIItem item;
    item.type = UIItemType::Bookmark;
    item.label = bm.name;
    item.color = bm.color;
    item.dataIndex = index;
    item.selected = selected;
    item.clickable = true;
    item.bounds = Rect(x, y, width, h);

    items.push_back(item);
    y += h + itemSpacing;
}

void PanelLayoutBuilder::addSeparator(const Color& color)
{
    UIItem item;
    item.type = UIItemType::Separator;
    item.color = color;
    item.bounds = Rect(x, y, width, 1);

    items.push_back(item);
    y += itemSpacing;
}

void PanelLayoutBuilder::addSpacer(int pixels)
{
    UIItem item;
    item.type = UIItemType::Spacer;
    item.bounds = Rect(x, y, width, pixels);

    items.push_back(item);
    y += pixels;
}

const Vector<UIItem>& PanelLayoutBuilder::getItems() const { return items; }
int PanelLayoutBuilder::getFinalHeight() const { return y; }

int PanelLayoutBuilder::hitTest(int mx, int my) const
{
    for (size_t i = 0; i < items.size(); i++)
    {
        const UIItem& it = items[i];
        if (!it.clickable) continue;

        if (mx >= it.bounds.x &&
            mx <= it.bounds.x + it.bounds.width &&
            my >= it.bounds.y &&
            my <= it.bounds.y + it.bounds.height)
        {
            return (int)i;
        }
    }
    return -1;
}
