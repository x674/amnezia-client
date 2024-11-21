#include "listViewFocusController.h"

#include <QQuickItem>
#include <QQueue>
#include <QPointF>
#include <QRectF>
#include <QQuickWindow>


bool isVisible(QObject* item)
{
    const auto res = item->property("visible").toBool();
    return res;
}

bool isFocusable(QObject* item)
{
    const auto res = item->property("isFocusable").toBool();
    return res;
}

QPointF getItemCenterPointOnScene(QQuickItem* item)
{
    const auto x0 = item->x() + (item->width() / 2);
    const auto y0 = item->y() + (item->height() / 2);
    return item->parentItem()->mapToScene(QPointF{x0, y0});
}

bool isLess(QObject* item1, QObject* item2)
{
    const auto p1 = getItemCenterPointOnScene(qobject_cast<QQuickItem*>(item1));
    const auto p2 = getItemCenterPointOnScene(qobject_cast<QQuickItem*>(item2));
    return (p1.y() == p2.y()) ? (p1.x() < p2.x()) : (p1.y() < p2.y());
}

bool isMore(QObject* item1, QObject* item2)
{
    return !isLess(item1, item2);
}

bool isEnabled(QObject* obj)
{
    const auto item = qobject_cast<QQuickItem*>(obj);
    return item && item->isEnabled();
}

QList<QObject*> getItemsChain(QObject* object)
{
    QList<QObject*> res;
    if (!object) {
        qDebug() << "The object is NULL";
        return res;
    }

    const auto children = object->children();

    for(const auto child : children) {
        if (child
            && isFocusable(child)
            && isEnabled(child)
            && isVisible(child)
            ) {
            res.append(child);
        } else {
            res.append(getItemsChain(child));
        }
    }
    return res;
}

void printItems(const QList<QObject*>& items, QObject* current_item)
{
    for(const auto& item : items) {
        QQuickItem* i = qobject_cast<QQuickItem*>(item);
        QPointF coords {getItemCenterPointOnScene(i)};
        QString prefix = current_item == i ? "==>" : "   ";
        qDebug() << prefix << " Item: " << i << " with coords: " << coords;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

ListViewFocusController::ListViewFocusController(QQuickItem* listView, QObject* parent)
    : QObject{parent}
    , m_listView{listView}
    , m_focusChain{}
    , m_currentSection{Section::Default}
    , m_header{nullptr}
    , m_footer{nullptr}
    , m_focusedItem{nullptr}
    , m_focusedItemIndex{-1}
    , m_delegateIndex{0}
    , m_isReturnNeeded{false}
    , m_currentSectionString {"Default", "Header", "Delegate", "Footer"}
{
    QVariant headerItemProperty = m_listView->property("headerItem");
    m_header = headerItemProperty.canConvert<QQuickItem*>() ? headerItemProperty.value<QQuickItem*>() : nullptr;

    QVariant footerItemProperty = m_listView->property("footerItem");
    m_footer = footerItemProperty.canConvert<QQuickItem*>() ? footerItemProperty.value<QQuickItem*>() : nullptr;
}

ListViewFocusController::~ListViewFocusController()
{

}

void ListViewFocusController::viewAtCurrentIndex() const
{
    switch(m_currentSection) {
    case Section::Default:
        [[fallthrough]];
    case Section::Header: {
        qDebug() << "===>> [FOCUS ON BEGINNING...]";
        QMetaObject::invokeMethod(m_listView, "positionViewAtBeginning");
        break;
    }
    case Section::Delegate: {
        qDebug() << "===>> [FOCUS ON INDEX...]";
        QMetaObject::invokeMethod(m_listView, "positionViewAtIndex",
                                  Q_ARG(int, m_delegateIndex),  // Index
                                  Q_ARG(int, 2));    // PositionMode (0 = Visible)
        break;
    }
    case Section::Footer: {
        qDebug() << "===>> [FOCUS ON END...]";
        QMetaObject::invokeMethod(m_listView, "positionViewAtEnd");
        break;
    }
    }

}

int ListViewFocusController::size() const
{
    return m_listView->property("count").toInt();
}

int ListViewFocusController::currentIndex() const
{
    return m_delegateIndex;
}

void ListViewFocusController::nextDelegate()
{
    const auto sectionName = m_currentSectionString[static_cast<int>(m_currentSection)];
    qDebug() << "===>> [nextDelegate... current section: " << sectionName << " ]";
    switch(m_currentSection) {
    case Section::Default: {
        if(hasHeader()) {
            m_currentSection = Section::Header;
            viewAtCurrentIndex();
            break;
        }
        [[fallthrough]];
    }
    case Section::Header: {
        if (size() > 0) {
            m_currentSection = Section::Delegate;
            viewAtCurrentIndex();
            break;
        }
        [[fallthrough]];
    }
    case Section::Delegate:
        if (m_delegateIndex < (size() - 1)) {
            m_delegateIndex++;
            viewAtCurrentIndex();
            break;
        } else if (hasFooter()) {
            m_currentSection = Section::Footer;
            viewAtCurrentIndex();
            break;
        }
        [[fallthrough]];
    case Section::Footer: {
        m_isReturnNeeded = true;
        m_currentSection = Section::Default;
        viewAtCurrentIndex();
        break;
    }
    default: {
        qCritical() << "Current section is invalid!";
        break;
    }
    }
}

void ListViewFocusController::previousDelegate()
{
    switch(m_currentSection) {
    case Section::Default: {
        if(hasFooter()) {
            m_currentSection = Section::Footer;
            break;
        }
        [[fallthrough]];
    }
    case Section::Footer: {
        if (size() > 0) {
            m_currentSection = Section::Delegate;
            m_delegateIndex = size() - 1;
            break;
        }
        [[fallthrough]];
    }
    case Section::Delegate: {
        if (m_delegateIndex > 0) {
            m_delegateIndex--;
            break;
        } else if (hasHeader()) {
            m_currentSection = Section::Header;
            break;
        }
        [[fallthrough]];
    }
    case Section::Header: {
        m_isReturnNeeded = true;
        m_currentSection = Section::Default;
        break;
    }
    default: {
        qCritical() << "Current section is invalid!";
        break;
    }
    }
}

void ListViewFocusController::decrementIndex()
{
    m_delegateIndex--;
}

QQuickItem* ListViewFocusController::itemAtIndex(const int index) const
{
    QQuickItem* item{nullptr};

    QMetaObject::invokeMethod(m_listView, "itemAtIndex",
                              Q_RETURN_ARG(QQuickItem*, item),
                              Q_ARG(int, index));

    return item;
}

QQuickItem* ListViewFocusController::currentDelegate() const
{
    QQuickItem* result{nullptr};

    switch(m_currentSection) {
    case Section::Default: {
        qWarning() << "No elements...";
        break;
    }
    case Section::Header: {
        result = m_header;
        break;
    }
    case Section::Delegate: {
        result = itemAtIndex(m_delegateIndex);
        break;
    }
    case Section::Footer: {
        result = m_footer;
        break;
    }
    }
    return result;
}

QQuickItem* ListViewFocusController::focusedItem() const
{
    return m_focusedItem;
}

void ListViewFocusController::focusNextItem()
{
    if (m_isReturnNeeded) {
        qDebug() << "===>> [ RETURN IS NEEDED... ]";
        return;
    }

    if (m_focusChain.empty()) {
        qDebug() << "Empty focusChain with current delegate: " << currentDelegate() << "Scanning for elements...";
        m_focusChain = getItemsChain(currentDelegate());
    }
    if (m_focusChain.empty()) {
        qWarning() << "No elements found in the delegate. Going to next delegate...";
        nextDelegate();
        focusNextItem();
        return;
    }
    m_focusedItemIndex++;
    m_focusedItem = qobject_cast<QQuickItem*>(m_focusChain.at(m_focusedItemIndex));
    qDebug() << "==>> [ Focused Item: " << m_focusedItem << " with Index: " << m_focusedItemIndex << " ]";
    m_focusedItem->forceActiveFocus(Qt::TabFocusReason);
}

void ListViewFocusController::focusPreviousItem()
{
    if (m_isReturnNeeded) {
        return;
    }

    if (m_focusChain.empty()) {
        qDebug() << "Empty focusChain with current delegate: " << currentDelegate() << "Scanning for elements...";
        m_focusChain = getItemsChain(currentDelegate());
    }
    if (m_focusChain.empty()) {
        qWarning() << "No elements found in the delegate. Going to next delegate...";
        previousDelegate();
        focusPreviousItem();
        return;
    }
    if (m_focusedItemIndex == -1) {
        m_focusedItemIndex = m_focusChain.size();
    }
    m_focusedItemIndex--;
    m_focusedItem = qobject_cast<QQuickItem*>(m_focusChain.at(m_focusedItemIndex));
    qDebug() << "==>> [ Focused Item: " << m_focusedItem << " with Index: " << m_focusedItemIndex << " ]";
    m_focusedItem->forceActiveFocus(Qt::TabFocusReason);
}

void ListViewFocusController::resetFocusChain()
{
    m_focusChain.clear();
    m_focusedItem = nullptr;
    m_focusedItemIndex = -1;
}

bool ListViewFocusController::isFirstFocusItemInDelegate() const
{
    return m_focusedItem && (m_focusedItem == m_focusChain.first());
}

bool ListViewFocusController::isLastFocusItemInDelegate() const
{
    return m_focusedItem && (m_focusedItem == m_focusChain.last());
}

bool ListViewFocusController::hasHeader() const
{
    return m_header && !getItemsChain(m_header).isEmpty();
}

bool ListViewFocusController::hasFooter() const
{
    return m_footer && !getItemsChain(m_footer).isEmpty();
}

bool ListViewFocusController::isFirstFocusItemInListView() const
{
    switch (m_currentSection) {
    case Section::Footer: {
        return isFirstFocusItemInDelegate() && !hasHeader() && (size() == 0);
    }
    case Section::Delegate: {
        return isFirstFocusItemInDelegate() && (m_delegateIndex == 0) && !hasHeader();
    }
    case Section::Header: {
        isFirstFocusItemInDelegate();
    }
    case Section::Default: {
        return true;
    }
    default:
        qWarning() << "Wrong section";
        return true;
    }
}

bool ListViewFocusController::isLastFocusItemInListView() const
{
    switch (m_currentSection) {
    case Section::Default: {
        return !hasHeader() && (size() == 0) && !hasFooter();
    }
    case Section::Header: {
        return isLastFocusItemInDelegate() && (size() == 0) && !hasFooter();
    }
    case Section::Delegate: {
        return isLastFocusItemInDelegate() && (m_delegateIndex == size() - 1) && !hasFooter();
    }
    case Section::Footer: {
        return isLastFocusItemInDelegate();
    }
    default:
        qWarning() << "Wrong section";
        return true;
    }
}

bool ListViewFocusController::isReturnNeeded() const
{
    return m_isReturnNeeded;
}
