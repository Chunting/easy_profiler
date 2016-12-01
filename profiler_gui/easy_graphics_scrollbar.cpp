/************************************************************************
* file name         : easy_graphics_scrollbar.cpp
* ----------------- :
* creation time     : 2016/07/04
* author            : Victor Zarubkin
* email             : v.s.zarubkin@gmail.com
* ----------------- :
* description       : .
* ----------------- :
* change log        : * 2016/07/04 Victor Zarubkin: Initial commit.
*                   :
*                   : *
* ----------------- :
* license           : Lightweight profiler library for c++
*                   : Copyright(C) 2016  Sergey Yagovtsev, Victor Zarubkin
*                   :
*                   :
*                   : Licensed under the Apache License, Version 2.0 (the "License");
*                   : you may not use this file except in compliance with the License.
*                   : You may obtain a copy of the License at
*                   :
*                   : http://www.apache.org/licenses/LICENSE-2.0
*                   :
*                   : Unless required by applicable law or agreed to in writing, software
*                   : distributed under the License is distributed on an "AS IS" BASIS,
*                   : WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*                   : See the License for the specific language governing permissions and
*                   : limitations under the License.
*                   :
*                   :
*                   : GNU General Public License Usage
*                   : Alternatively, this file may be used under the terms of the GNU
*                   : General Public License as published by the Free Software Foundation,
*                   : either version 3 of the License, or (at your option) any later version.
*                   :
*                   : This program is distributed in the hope that it will be useful,
*                   : but WITHOUT ANY WARRANTY; without even the implied warranty of
*                   : MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
*                   : GNU General Public License for more details.
*                   :
*                   : You should have received a copy of the GNU General Public License
*                   : along with this program.If not, see <http://www.gnu.org/licenses/>.
************************************************************************/

#include <algorithm>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include "easy_graphics_scrollbar.h"
#include "globals.h"

//////////////////////////////////////////////////////////////////////////

const int DEFAULT_TOP = -40;
const int DEFAULT_HEIGHT = 80;
const int INDICATOR_SIZE = 6;
const int INDICATOR_SIZE_x2 = INDICATOR_SIZE << 1;

//////////////////////////////////////////////////////////////////////////

inline qreal clamp(qreal _minValue, qreal _value, qreal _maxValue)
{
    return (_value < _minValue ? _minValue : (_value > _maxValue ? _maxValue : _value));
}

inline qreal sqr(qreal _value)
{
    return _value * _value;
}

inline qreal calculate_color1(qreal h, qreal k)
{
    return ::std::min(h * k, 0.9999999);
}

inline qreal calculate_color2(qreal h, qreal k)
{
    return ::std::min(sqr(sqr(h)) * k, 0.9999999);
}

//////////////////////////////////////////////////////////////////////////

EasyGraphicsSliderItem::EasyGraphicsSliderItem(bool _main) : Parent(), m_halfwidth(0), m_bMain(_main)
{
    m_indicator.reserve(3);

    if (_main)
    {
        m_indicator.push_back(QPointF(0, DEFAULT_TOP + INDICATOR_SIZE));
        m_indicator.push_back(QPointF(-INDICATOR_SIZE, DEFAULT_TOP));
        m_indicator.push_back(QPointF(INDICATOR_SIZE, DEFAULT_TOP));
    }
    else
    {
        m_indicator.push_back(QPointF(0, DEFAULT_TOP + DEFAULT_HEIGHT - INDICATOR_SIZE));
        m_indicator.push_back(QPointF(-INDICATOR_SIZE, DEFAULT_TOP + DEFAULT_HEIGHT));
        m_indicator.push_back(QPointF(INDICATOR_SIZE, DEFAULT_TOP + DEFAULT_HEIGHT));
    }

    setWidth(1);
    setBrush(Qt::SolidPattern);
}

EasyGraphicsSliderItem::~EasyGraphicsSliderItem()
{

}

void EasyGraphicsSliderItem::paint(QPainter* _painter, const QStyleOptionGraphicsItem* _option, QWidget* _widget)
{
    if (static_cast<const EasyGraphicsScrollbar*>(scene()->parent())->bindMode())
    {
        return;
    }

    const auto currentScale = static_cast<const EasyGraphicsScrollbar*>(scene()->parent())->getWindowScale();
    const auto br = rect();

    qreal w = width() * currentScale;
    QRectF r(br.left() * currentScale, br.top() + INDICATOR_SIZE, w, br.height() - INDICATOR_SIZE_x2);
    const auto r_right = r.right();
    const auto r_bottom = r.bottom();
    auto b = brush();

    _painter->save();
    _painter->setTransform(QTransform::fromScale(1.0 / currentScale, 1), true);
    _painter->setBrush(b);
    
    if (w > 1)
    {
        _painter->setPen(Qt::NoPen);
        _painter->drawRect(r);

        // Draw left and right borders
        auto cmode = _painter->compositionMode();
        if (m_bMain) _painter->setCompositionMode(QPainter::CompositionMode_Exclusion);
        _painter->setPen(QColor::fromRgba(0xe0000000 | b.color().rgb()));
        _painter->drawLine(QPointF(r.left(), r.top()), QPointF(r.left(), r_bottom));
        _painter->drawLine(QPointF(r_right, r.top()), QPointF(r_right, r_bottom));
        if (!m_bMain) _painter->setCompositionMode(cmode);
    }
    else
    {
        _painter->setPen(QColor::fromRgba(0xe0000000 | b.color().rgb()));
        _painter->drawLine(QPointF(r.left(), r.top()), QPointF(r.left(), r_bottom));
        if (m_bMain) _painter->setCompositionMode(QPainter::CompositionMode_Exclusion);
    }

    // Draw triangle indicators for small slider
    _painter->setTransform(QTransform::fromTranslate(r.left() + w * 0.5, 0), true);
    _painter->setPen(b.color().rgb());
    _painter->drawPolygon(m_indicator);

    _painter->restore();
}

qreal EasyGraphicsSliderItem::width() const
{
    return m_halfwidth * 2.0;
}

qreal EasyGraphicsSliderItem::halfwidth() const
{
    return m_halfwidth;
}

void EasyGraphicsSliderItem::setWidth(qreal _width)
{
    m_halfwidth = _width * 0.5;
    setRect(-m_halfwidth, DEFAULT_TOP, _width, DEFAULT_HEIGHT);
}

void EasyGraphicsSliderItem::setHalfwidth(qreal _halfwidth)
{
    m_halfwidth = _halfwidth;
    setRect(-m_halfwidth, DEFAULT_TOP, m_halfwidth * 2.0, DEFAULT_HEIGHT);
}

void EasyGraphicsSliderItem::setColor(QRgb _color)
{
    setColor(QColor::fromRgba(_color));
}

void EasyGraphicsSliderItem::setColor(const QColor& _color)
{
    auto b = brush();
    b.setColor(_color);
    setBrush(b);
}

//////////////////////////////////////////////////////////////////////////

EasyMinimapItem::EasyMinimapItem() : Parent(nullptr), m_pSource(nullptr), m_maxDuration(0), m_minDuration(0), m_threadId(0), m_timeUnits(::profiler_gui::TimeUnits_auto)
{

}

EasyMinimapItem::~EasyMinimapItem()
{

}

QRectF EasyMinimapItem::boundingRect() const
{
    return m_boundingRect;
}

void EasyMinimapItem::paint(QPainter* _painter, const QStyleOptionGraphicsItem* _option, QWidget* _widget)
{
    if (m_pSource == nullptr)
    {
        return;
    }

    const auto widget = static_cast<const EasyGraphicsScrollbar*>(scene()->parent());
    const bool bindMode = widget->bindMode();
    const auto currentScale = widget->getWindowScale();
    const auto bottom = m_boundingRect.bottom();
    const auto width = m_boundingRect.width() * currentScale;
    const auto coeff = m_boundingRect.height() / (m_maxDuration - m_minDuration);
    const auto heightRevert = 1.0 / m_boundingRect.height();

    QRectF rect;
    QBrush brush(Qt::SolidPattern);
    QRgb previousColor = 0;

    //brush.setColor(QColor::fromRgba(0x80808080));

    _painter->save();
    _painter->setPen(Qt::NoPen);
    //_painter->setBrush(brush);
    _painter->setTransform(QTransform::fromScale(1.0 / currentScale, 1), true);

    const bool gotFrame = EASY_GLOBALS.frame_time > 1e-6f;
    qreal frameCoeff = 1;
    if (gotFrame)
    {
        if (EASY_GLOBALS.frame_time <= m_minDuration)
            frameCoeff = 0;
        else
            frameCoeff = 0.9 * (m_maxDuration - m_minDuration) / (EASY_GLOBALS.frame_time - m_minDuration);
    }

    auto const calculate_color = gotFrame ? calculate_color2 : calculate_color1;
    auto const k = gotFrame ? sqr(sqr(heightRevert * frameCoeff)) : heightRevert;
    auto& items = *m_pSource;

    if (!items.empty())
    {
        qreal previous_x = -1e30, previous_h = -1e30, offset = 0.;
        auto first = items.begin();
        auto realScale = currentScale;
        auto minimum = widget->minimum();
        auto maximum = widget->maximum();

        if (bindMode)
        {
            const auto range = widget->sliderWidth();
            minimum = widget->value();
            maximum = minimum + range;
            realScale *= widget->range() / range;
            offset = minimum * realScale;

            auto first = ::std::lower_bound(items.begin(), items.end(), minimum, [](const ::profiler_gui::EasyBlockItem& _item, qreal _value)
            {
                return _item.left() < _value;
            });

            if (first != items.end())
            {
                if (first != items.begin())
                    --first;
            }
            else
            {
                first = items.begin() + items.size() - 1;
            }
        }

        for (auto it = first, end = items.end(); it != end; ++it)
        {
            // Draw rectangle

            if (it->left() > maximum)
                break;

            if (it->right() < minimum)
                continue;

            const qreal item_x = it->left() * realScale - offset;
            const qreal item_w = ::std::max(it->width() * realScale, 1.0);
            const qreal item_r = item_x + item_w;
            const auto h = ::std::max((it->width() - m_minDuration) * coeff, 2.0);

            if (h < previous_h && item_r < previous_x)
                continue;

            const auto col = calculate_color(h, k);
            const auto color = 0x00ffffff & QColor::fromHsvF((1.0 - col) * 0.375, 0.85, 0.85).rgb();

            if (previousColor != color)
            {
                // Set background color brush for rectangle
                previousColor = color;
                brush.setColor(QColor::fromRgba(0xc0000000 | color));
                _painter->setBrush(brush);
            }

            rect.setRect(item_x, bottom - h, item_w, h);
            _painter->drawRect(rect);

            previous_x = item_r;
            previous_h = h;
        }
    }

    qreal top_width = width, bottom_width = width;
    int font_h = 0;
    if (!m_maxDurationStr.isEmpty())
    {
        rect.setRect(0, m_boundingRect.top() - INDICATOR_SIZE, width - 3, m_boundingRect.height() + INDICATOR_SIZE_x2);

        if (m_timeUnits != EASY_GLOBALS.time_units)
        {
            m_timeUnits = EASY_GLOBALS.time_units;
            m_maxDurationStr = ::profiler_gui::timeStringReal(m_timeUnits, m_maxDuration, 3);
            m_minDurationStr = ::profiler_gui::timeStringReal(m_timeUnits, m_minDuration, 3);
        }

        auto fm = _painter->fontMetrics();
        font_h = fm.height();
        bottom_width -= fm.width(m_minDurationStr) + 6;
        top_width -= fm.width(m_maxDurationStr) + 6;

        _painter->setPen(Qt::black);
        _painter->drawText(rect, Qt::AlignRight | Qt::AlignTop, m_maxDurationStr);
        _painter->drawText(rect, Qt::AlignRight | Qt::AlignBottom, m_minDurationStr);
    }

    _painter->setPen(Qt::darkGray);
    _painter->drawLine(QLineF(0, bottom, bottom_width, bottom));
    _painter->drawLine(QLineF(0, m_boundingRect.top(), top_width, m_boundingRect.top()));

    if (m_minDuration < EASY_GLOBALS.frame_time && EASY_GLOBALS.frame_time < m_maxDuration)
    {
        // Draw marker displaying required frame_time step
        const auto h = bottom - (EASY_GLOBALS.frame_time - m_minDuration) * coeff;
        _painter->setPen(Qt::DashLine);

        auto w = width;
        const auto boundary = INDICATOR_SIZE - font_h;
        if (h < (m_boundingRect.top() - boundary))
            w = top_width;
        else if (h > (bottom + boundary))
            w = bottom_width;

        _painter->drawLine(QLineF(0, h, w, h));
    }

    _painter->restore();
}

::profiler::thread_id_t EasyMinimapItem::threadId() const
{
    return m_threadId;
}

void EasyMinimapItem::setBoundingRect(const QRectF& _rect)
{
    m_boundingRect = _rect;
}

void EasyMinimapItem::setBoundingRect(qreal x, qreal y, qreal w, qreal h)
{
    m_boundingRect.setRect(x, y, w, h);
}

void EasyMinimapItem::setSource(::profiler::thread_id_t _thread_id, const ::profiler_gui::EasyItems* _items)
{
    m_pSource = _items;
    m_threadId = _thread_id;

    if (m_pSource != nullptr)
    {
        if (m_pSource->empty())
        {
            m_pSource = nullptr;
        }
        else
        {
            m_maxDuration = 0;
            m_minDuration = 1e30;
            for (const auto& item : *m_pSource)
            {
                auto w = item.width();

                if (w > m_maxDuration)
                {
                    m_maxDuration = item.width();
                }

                if (w < m_minDuration)
                {
                    m_minDuration = w;
                }
            }
        }
    }

    if (m_pSource == nullptr)
    {
        m_maxDurationStr.clear();
        m_minDurationStr.clear();
        hide();
    }
    else
    {
        m_timeUnits = EASY_GLOBALS.time_units;
        m_maxDurationStr = ::profiler_gui::timeStringReal(m_timeUnits, m_maxDuration, 3);
        m_minDurationStr = ::profiler_gui::timeStringReal(m_timeUnits, m_minDuration, 3);
        show();
    }
}

//////////////////////////////////////////////////////////////////////////

EasyGraphicsScrollbar::EasyGraphicsScrollbar(QWidget* _parent)
    : Parent(_parent)
    , m_minimumValue(0)
    , m_maximumValue(500)
    , m_value(10)
    , m_windowScale(1)
    , m_mouseButtons(Qt::NoButton)
    , m_slider(nullptr)
    , m_chronometerIndicator(nullptr)
    , m_minimap(nullptr)
    , m_bScrolling(false)
    , m_bBindMode(false)
{
    setCacheMode(QGraphicsView::CacheNone);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    //setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setOptimizationFlag(QGraphicsView::DontSavePainterState, true);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setContentsMargins(0, 0, 0, 0);
    setFixedHeight(DEFAULT_HEIGHT + 2);

    auto selfScene = new QGraphicsScene(this);
    selfScene->setSceneRect(0, DEFAULT_TOP, 500, DEFAULT_HEIGHT);
    setScene(selfScene);

    m_minimap = new EasyMinimapItem();
    m_minimap->setPos(0, 0);
    m_minimap->setBoundingRect(0, DEFAULT_TOP + INDICATOR_SIZE, scene()->width(), DEFAULT_HEIGHT - INDICATOR_SIZE_x2);
    selfScene->addItem(m_minimap);
    m_minimap->hide();

    m_chronometerIndicator = new EasyGraphicsSliderItem(false);
    m_chronometerIndicator->setPos(0, 0);
    m_chronometerIndicator->setColor(0x40000000 | ::profiler_gui::CHRONOMETER_COLOR.rgba());
    selfScene->addItem(m_chronometerIndicator);
    m_chronometerIndicator->hide();

    m_slider = new EasyGraphicsSliderItem(true);
    m_slider->setPos(0, 0);
    m_slider->setColor(0x40c0c0c0);
    selfScene->addItem(m_slider);
    m_slider->hide();

    connect(&EASY_GLOBALS.events, &::profiler_gui::EasyGlobalSignals::timelineMarkerChanged, [this](){ scene()->update(); });

    centerOn(0, 0);
}

EasyGraphicsScrollbar::~EasyGraphicsScrollbar()
{

}

//////////////////////////////////////////////////////////////////////////

void EasyGraphicsScrollbar::clear()
{
    setMinimapFrom(0, nullptr);
    hideChrono();
    setRange(0, 100);
    setSliderWidth(2);
    setValue(0);
}

//////////////////////////////////////////////////////////////////////////

bool EasyGraphicsScrollbar::bindMode() const
{
    return m_bBindMode;
}

qreal EasyGraphicsScrollbar::getWindowScale() const
{
    return m_windowScale;
}

::profiler::thread_id_t EasyGraphicsScrollbar::minimapThread() const
{
    return m_minimap->threadId();
}

qreal EasyGraphicsScrollbar::minimum() const
{
    return m_minimumValue;
}

qreal EasyGraphicsScrollbar::maximum() const
{
    return m_maximumValue;
}

qreal EasyGraphicsScrollbar::range() const
{
    return m_maximumValue - m_minimumValue;
}

qreal EasyGraphicsScrollbar::value() const
{
    return m_value;
}

qreal EasyGraphicsScrollbar::sliderWidth() const
{
    return m_slider->width();
}

qreal EasyGraphicsScrollbar::sliderHalfWidth() const
{
    return m_slider->halfwidth();
}

//////////////////////////////////////////////////////////////////////////

void EasyGraphicsScrollbar::setValue(qreal _value)
{
    m_value = clamp(m_minimumValue, _value, ::std::max(m_minimumValue, m_maximumValue - m_slider->width()));
    m_slider->setX(m_value + m_slider->halfwidth());
    emit valueChanged(m_value);
}

void EasyGraphicsScrollbar::setRange(qreal _minValue, qreal _maxValue)
{
    const auto oldRange = range();
    const auto oldValue = oldRange < 1e-3 ? 0.0 : m_value / oldRange;

    m_minimumValue = _minValue;
    m_maximumValue = _maxValue;
    scene()->setSceneRect(_minValue, DEFAULT_TOP, _maxValue - _minValue, DEFAULT_HEIGHT);
    m_minimap->setBoundingRect(_minValue, DEFAULT_TOP + INDICATOR_SIZE, _maxValue, DEFAULT_HEIGHT - INDICATOR_SIZE_x2);
    emit rangeChanged();

    setValue(_minValue + oldValue * range());
    onWindowWidthChange(width());
}

void EasyGraphicsScrollbar::setSliderWidth(qreal _width)
{
    m_slider->setWidth(_width);
    setValue(m_value);
}

//////////////////////////////////////////////////////////////////////////

void EasyGraphicsScrollbar::setChronoPos(qreal _left, qreal _right)
{
    m_chronometerIndicator->setWidth(_right - _left);
    m_chronometerIndicator->setX(_left + m_chronometerIndicator->halfwidth());
}

void EasyGraphicsScrollbar::showChrono()
{
    m_chronometerIndicator->show();
}

void EasyGraphicsScrollbar::hideChrono()
{
    m_chronometerIndicator->hide();
}

//////////////////////////////////////////////////////////////////////////

void EasyGraphicsScrollbar::setMinimapFrom(::profiler::thread_id_t _thread_id, const ::profiler_gui::EasyItems* _items)
{
    m_minimap->setSource(_thread_id, _items);
    m_slider->setVisible(m_minimap->isVisible());
    scene()->update();
}

//////////////////////////////////////////////////////////////////////////

void EasyGraphicsScrollbar::mousePressEvent(QMouseEvent* _event)
{
    m_mouseButtons = _event->buttons();

    if (m_mouseButtons & Qt::RightButton)
    {
        m_bBindMode = !m_bBindMode;
        scene()->update();
    }

    if (m_mouseButtons & Qt::LeftButton)
    {
        m_bScrolling = true;
        m_mousePressPos = _event->pos();
        if (!m_bBindMode)
            setValue(mapToScene(m_mousePressPos).x() - m_minimumValue - m_slider->halfwidth());
    }

    _event->accept();
    //QGraphicsView::mousePressEvent(_event);
}

void EasyGraphicsScrollbar::mouseReleaseEvent(QMouseEvent* _event)
{
    m_mouseButtons = _event->buttons();
    m_bScrolling = false;
    _event->accept();
    //QGraphicsView::mouseReleaseEvent(_event);
}

void EasyGraphicsScrollbar::mouseMoveEvent(QMouseEvent* _event)
{
    if (m_mouseButtons & Qt::LeftButton)
    {
        const auto pos = _event->pos();
        const auto delta = pos - m_mousePressPos;
        m_mousePressPos = pos;

        if (m_bScrolling)
        {
            auto realScale = m_windowScale;
            if (m_bBindMode)
                realScale *= -range() / sliderWidth();
            setValue(m_value + delta.x() / realScale);
        }
    }
}

void EasyGraphicsScrollbar::wheelEvent(QWheelEvent* _event)
{
    _event->accept();

    if (!m_bBindMode)
    {
        const auto w = m_slider->halfwidth() * (_event->delta() < 0 ? ::profiler_gui::SCALING_COEFFICIENT : ::profiler_gui::SCALING_COEFFICIENT_INV);
        setValue(mapToScene(_event->pos()).x() - m_minimumValue - w);
        emit wheeled(w * m_windowScale, _event->delta());
    }
    else
    {
        const auto x = (mapToScene(_event->pos()).x() - m_minimumValue) * m_windowScale;
        emit wheeled(x, _event->delta());
    }
}

void EasyGraphicsScrollbar::resizeEvent(QResizeEvent* _event)
{
    onWindowWidthChange(_event->size().width());
}

//////////////////////////////////////////////////////////////////////////
/*
void EasyGraphicsScrollbar::contextMenuEvent(QContextMenuEvent* _event)
{
    if (EASY_GLOBALS.profiler_blocks.empty())
    {
        return;
    }

    QMenu menu;

    for (const auto& it : EASY_GLOBALS.profiler_blocks)
    {
        QString label;
        if (it.second.got_name())
            label = ::std::move(QString("%1 Thread %2").arg(it.second.name()).arg(it.first));
        else
            label = ::std::move(QString("Thread %1").arg(it.first));

        auto action = new QAction(label, nullptr);
        action->setData(it.first);
        action->setCheckable(true);
        action->setChecked(it.first == EASY_GLOBALS.selected_thread);
        connect(action, &QAction::triggered, this, &This::onThreadActionClicked);

        menu.addAction(action);
    }

    menu.exec(QCursor::pos());
    _event->accept();
}
*/
//////////////////////////////////////////////////////////////////////////

void EasyGraphicsScrollbar::onThreadActionClicked(bool)
{
    auto action = qobject_cast<QAction*>(sender());
    if (action == nullptr)
        return;

    const auto thread_id = action->data().toUInt();
    if (thread_id != EASY_GLOBALS.selected_thread)
    {
        EASY_GLOBALS.selected_thread = thread_id;
        emit EASY_GLOBALS.events.selectedThreadChanged(thread_id);
    }
}

//////////////////////////////////////////////////////////////////////////

void EasyGraphicsScrollbar::onWindowWidthChange(qreal _width)
{
    const auto oldScale = m_windowScale;
    const auto scrollingRange = range();

    if (scrollingRange < 1e-3)
    {
        m_windowScale = 1;
    }
    else
    {
        m_windowScale = _width / scrollingRange;
    }

    scale(m_windowScale / oldScale, 1);
}

//////////////////////////////////////////////////////////////////////////
