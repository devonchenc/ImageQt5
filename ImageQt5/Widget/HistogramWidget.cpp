#include "HistogramWidget.h"

#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include "../GlobalFunc.h"
#include "HistogramProcessor.h"

#define DRAG_NONE		-1
#define DRAG_CURSOR0	0
#define DRAG_CURSOR1	1
#define DRAG_CURSOR2	2
#define DRAG_HISTOGRAM	3

HistogramWidget::HistogramWidget(QWidget* parent)
	: QWidget(parent)
	, _heightArray(nullptr)
	, _minHeight(0)
	, _maxHeight(0)
	, _select(nullptr)
	, _selectTemp(nullptr)
	, _drag(DRAG_NONE)
	, _bottom(0)
	, _top(0)
	, _mid(1.0f)
	, _minValue(0)
	, _maxValue(0)
{
	allocateMemory();
}

HistogramWidget::~HistogramWidget()
{
	if (_heightArray)
	{
		delete _heightArray;
	}
	if (_select)
	{
		delete _select;
	}
	if (_selectTemp)
	{
		delete _selectTemp;
	}
}

void HistogramWidget::init()
{
	generateHistogram();
}

void HistogramWidget::reset()
{
	repaint();
}

QSize HistogramWidget::sizeHint() const
{
	return QSize(200, 150);
}

QSize HistogramWidget::minimumSizeHint() const
{
	return QSize(200, 150);
}

void HistogramWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);

	QSize size = event->size();
	int oldWidth = _rectHistogram.width();
	_rectHistogram = QRect(HS_MARGIN, 0, size.width() - HS_MARGIN * 2, size.height() - HS_MARGIN * 2);

	allocateMemory(oldWidth);

	generateHistogram();

	if (_maxValue != _minValue)
	{
		float factor = (_bottom - _minValue) / (_maxValue - _minValue);
		factor = factor < 0 ? 0 : factor;
		_cursorPos[0] = round(factor * _rectHistogram.width());

		factor = (_top - _minValue) / (_maxValue - _minValue);
		factor = factor > 1 ? 1 : factor;
		_cursorPos[2] = round(factor * _rectHistogram.width());

		_cursorPos[1] = round((_cursorPos[2] - _cursorPos[0]) / (1.0f + _mid)) + _cursorPos[0];
	}
	else
	{
		_cursorPos[0] = 0;
		_cursorPos[1] = round(float(_rectHistogram.width()) / (1.0f + _mid));
		_cursorPos[2] = _rectHistogram.width();
	}
	
	repaint();

	// Emit signal to parent widget
	emit resize();
}

void HistogramWidget::paintEvent(QPaintEvent* /*event*/)
{
	paintCursor();
	paintHistogram();
}

void HistogramWidget::paintCursor()
{
	int left = _rectHistogram.left();
	int height = _rectHistogram.bottom();

	QPoint pt[3];
	pt[0].setX(_cursorPos[0] + left);
	pt[0].setY(height);
	pt[1].setX(_cursorPos[0] + left - CURSOR_SIZE);
	pt[1].setY(pt[0].y() + CURSOR_SIZE);
	pt[2].setX(_cursorPos[0] + left + CURSOR_SIZE);
	pt[2].setY(pt[0].y() + CURSOR_SIZE);
	QPolygon polygon;
	polygon << pt[0] << pt[1] << pt[2];
	QPainter painter(this);
	painter.drawPolygon(polygon);

	pt[0].setX(_cursorPos[1] + left);
	pt[0].setY(height);
	pt[1].setX(_cursorPos[1] + left - CURSOR_SIZE);
	pt[1].setY(pt[0].y() + CURSOR_SIZE);
	pt[2].setX(_cursorPos[1] + left + CURSOR_SIZE);
	pt[2].setY(pt[0].y() + CURSOR_SIZE);
	polygon.clear();
	polygon << pt[0] << pt[1] << pt[2];
	painter.drawPolygon(polygon);

	pt[0].setX(_cursorPos[2] + left);
	pt[0].setY(height);
	pt[1].setX(_cursorPos[2] + left - CURSOR_SIZE);
	pt[1].setY(pt[0].y() + CURSOR_SIZE);
	pt[2].setX(_cursorPos[2] + left + CURSOR_SIZE);
	pt[2].setY(pt[0].y() + CURSOR_SIZE);
	polygon.clear();
	polygon << pt[0] << pt[1] << pt[2];
	painter.drawPolygon(polygon);
}

void HistogramWidget::paintHistogram()
{
	if (getGlobalDocument() == nullptr)
		return;
	
	if (_maxHeight == 0)
		return;

	QRect rect = this->rect();

	// Draw background
	QPen newHighlightPen(qRgb(92, 92, 92));
	QPainter painter(this);
	painter.setPen(newHighlightPen);
	for (int i = 0; i < _rectHistogram.width(); i++)
	{
		if (_select[i] || _selectTemp[i])
		{
			painter.drawLine(i + _rectHistogram.left(), _rectHistogram.top(),
				i + _rectHistogram.left(), _rectHistogram.bottom());
		}
	}

	// Draw histogram
	QPen newPen(qRgb(0, 255, 0));
	QPen newPen2(qRgb(81, 183, 255));
	for (int i = 0; i < _rectHistogram.width(); i++)
	{
		if (_select[i] || _selectTemp[i])
		{
			painter.setPen(newPen2);
		}
		else
		{
			painter.setPen(newPen);
		}

		painter.drawLine(i + _rectHistogram.left(), _rectHistogram.bottom(),
						i + _rectHistogram.left(), _rectHistogram.bottom() - indexToHeight(i));
	}
}

void HistogramWidget::mousePressEvent(QMouseEvent* event)
{
	QPoint point = event->pos();
	if (event->button() == Qt::LeftButton)
	{
		if (QApplication::keyboardModifiers() != Qt::ControlModifier)
		{
			memset(_select, 0, sizeof(bool) * _rectHistogram.width());
		}

		_start = _finish = point.x();
		_drag = DRAG_HISTOGRAM;

		repaint();
	}
}

void HistogramWidget::mouseMoveEvent(QMouseEvent* event)
{
	QPoint point = event->pos();

	if ((event->buttons() & Qt::LeftButton) == false)
		return;

	_finish = qMax(point.x(), _rectHistogram.left());
	_finish = qMin(_finish, _rectHistogram.right() - 1);

	// ͳ����ʱ��ѡ�еķ�Χ
	calcSelectTempArea();

	repaint();
}

void HistogramWidget::mouseReleaseEvent(QMouseEvent* event)
{
	// ͳ�Ʊ�ѡ�еķ�Χ
	calcSelectArea();

	// ���ڴ���
//	setBottomAndTop(_select, _rectHistogram.width());

	emit updateImage();
}

void HistogramWidget::allocateMemory(int oldWidth)
{
	if (_heightArray)
	{
		delete _heightArray;
	}
	_heightArray = new uint[256];
	memset(_heightArray, 0, sizeof(uint) * 256);

	if (_rectHistogram.width() != oldWidth)
	{
		if (oldWidth != 0 && _select && _selectTemp)
		{
			memcpy(_selectTemp, _select, sizeof(bool) * oldWidth);
		}

		if (_select)
		{
			delete _select;
		}
		int histogramWidth = _rectHistogram.width();
		_select = new bool[histogramWidth];
		memset(_select, 0, sizeof(bool) * histogramWidth);
		if (_selectTemp)
		{
			// Copy select array from _selectTemp
			copySelectArray(_selectTemp, oldWidth);
		}

		if (_selectTemp)
		{
			delete _selectTemp;
		}
		_selectTemp = new bool[histogramWidth];
		memset(_selectTemp, 0, sizeof(bool) * histogramWidth);
	}

//	if (m_pWindowProcessor)
//	{
//		m_pWindowProcessor->SetWindowArray(m_pSelect, &m_nHistogramWidth);
//	}
}

void HistogramWidget::copySelectArray(bool* array, int arrayNum)
{
	int histogramWidth = _rectHistogram.width();
	assert(histogramWidth > 1);
	// Make sure width has been changed
	if (histogramWidth != arrayNum)
	{
		for (int i = 0; i < histogramWidth; i++)
		{
			// Interpolation
			int index = round(float(i) * (arrayNum - 1) / (histogramWidth - 1));
			_select[i] = array[index];
		}
	}
	else
	{
		memcpy(_select, array, sizeof(bool) * arrayNum);
	}
}

void HistogramWidget::generateHistogram()
{
	BaseImage* image = getGlobalImage();
	if (image == nullptr)
		return;

	memcpy(_heightArray, image->getGrayPixelArray(), sizeof(uint) * 256);

	_minHeight = _maxHeight = _heightArray[0];
	// Find max and min value in _heightArray
	for (int i = 0; i < 256; i++)
	{
		if (_maxHeight < _heightArray[i])
		{
			_maxHeight = _heightArray[i];
		}
		if (_minHeight > _heightArray[i])
		{
			_minHeight = _heightArray[i];
		}
	}
	// ��ֹͼ�����������ݾ�Ϊ0���������
	if (_maxHeight == 0 && _minHeight == 0)
		return;

	// ��Ը߶�����Ϊ���Ը߶�
	for (int i = 0; i < 256; i++)
	{
		_heightArray[i] = (_heightArray[i] - _minHeight) * (_rectHistogram.height() - 1) / (_maxHeight - _minHeight) + 1;
	}

	repaint();
}

void HistogramWidget::calcSelectArea()
{
	for (int i = 0; i < _rectHistogram.width(); i++)
	{
		if (_selectTemp[i])
		{
			_select[i] = true;
		}
	}

	// Rest temp select array
	memset(_selectTemp, 0, sizeof(bool) * _rectHistogram.width());
}

// ͳ����ʱ��ѡ�еķ�Χ
void HistogramWidget::calcSelectTempArea()
{
	int bottom = qMin(_start, _finish) - _rectHistogram.left();
	int top = qMax(_start, _finish) - _rectHistogram.left();

	for (int i = 0; i < _rectHistogram.width(); i++)
	{
		if (i >= bottom && i <= top)
		{
			_selectTemp[i] = true;
		}
		else
		{
			_selectTemp[i] = false;
		}
	}
}
// Change index to height
int HistogramWidget::indexToHeight(int i)
{
	float pos = float(i) * 255.0f / (_rectHistogram.width() - 1);
	int integer = int(floor(pos));
	float fraction = pos - floor(pos);

	if (integer == 255)
	{
		return _heightArray[255];
	}
	else
	{
		float height = (1.0f - fraction) * _heightArray[integer] + fraction * _heightArray[integer + 1];
		return round(height);
	}
}