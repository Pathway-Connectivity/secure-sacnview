// Copyright (c) 2015 Tom Barthel-Steer, http://www.tomsteer.net
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "universedisplay.h"
#include "preferences.h"
#include <QPainter>
#include <QMouseEvent>

#define FIRST_COL_WIDTH 60
#define ROW_COUNT 16
#define COL_COUNT 32
#define CELL_HEIGHT 18
#define CELL_WIDTH 25

static const QColor gridLineColor = QColor::fromRgb(0xc0, 0xc0, 0xc0);
static const QColor textColor = QColor::fromRgb(0x0, 0x0, 0x0);

UniverseDisplay::UniverseDisplay(QWidget *parent) : QWidget(parent)
{
    m_listener = 0;
    for(int i=0; i<512; i++)
        m_sources << sACNMergedAddress();
    setMinimumWidth(FIRST_COL_WIDTH + COL_COUNT * 12);
    m_selectedAddress = -1;
}

void UniverseDisplay::setUniverse(int universe)
{
    m_listener = sACNManager::getInstance()->getListener(universe);
    connect(m_listener, SIGNAL(levelsChanged()), this, SLOT(levelsChanged()));
}

void UniverseDisplay::levelsChanged()
{
    if(m_listener)
    {
        m_sources = m_listener->mergedLevels();
        update();
    }
}

QSize UniverseDisplay::minimumSizeHint() const
{
    return sizeHint();
}

QSize UniverseDisplay::sizeHint() const
{
    return QSize( FIRST_COL_WIDTH + CELL_WIDTH * COL_COUNT, CELL_HEIGHT * (ROW_COUNT + 1));
}

void UniverseDisplay::paintEvent(QPaintEvent *event)
{
    QElapsedTimer t;
    t.start();
    Q_UNUSED(event);
    QPainter painter(this);

    qreal wantedHeight = CELL_HEIGHT * (ROW_COUNT + 1);
    qreal wantedWidth = FIRST_COL_WIDTH + CELL_WIDTH * COL_COUNT;

    qreal scaleWidth = width()/wantedWidth;
    qreal scaleHeight = height()/ wantedHeight;
    qreal minScale = qMin(scaleWidth, scaleHeight);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate((width()-minScale*wantedWidth) /2,0);
    painter.scale(minScale,minScale);



    painter.fillRect(QRectF(0,0, wantedWidth, wantedHeight), QBrush(QColor("#FFF")));

    painter.setPen(textColor);
    painter.setFont(QFont("Segoe UI", 8));
    for(int row=1; row<ROW_COUNT+1; row++)
    {
        QRect textRect(0, row*CELL_HEIGHT, FIRST_COL_WIDTH, CELL_HEIGHT);
        QString rowLabel = QString("%1 - %2")
                .arg(1+(row-1)*32)
                .arg((row)*32);
        painter.drawText(textRect, rowLabel, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
    }
    for(int col=0; col<COL_COUNT; col++)
    {
        QRect textRect(FIRST_COL_WIDTH + col*CELL_WIDTH, 0, CELL_WIDTH, CELL_HEIGHT);
        QString rowLabel = QString("%1")
                .arg(col+1);
        painter.drawText(textRect, rowLabel, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
    }
    for(int row=0; row<ROW_COUNT; row++)
        for(int col=0; col<COL_COUNT; col++)
        {
            int address = row*COL_COUNT + col;
            QRect textRect(FIRST_COL_WIDTH + col*CELL_WIDTH, (row+1)*CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT);
            sACNMergedAddress addressInfo = m_sources[address];

            if(addressInfo.winningSource)
            {
                QColor fillColor = Preferences::getInstance()->colorForCID(addressInfo.winningSource->src_cid);

                QString rowLabel = QString("%1")
                        .arg(addressInfo.level);
                painter.fillRect(textRect, fillColor);
                painter.drawText(textRect, rowLabel, QTextOption(Qt::AlignHCenter | Qt::AlignVCenter));
            }

        }

    painter.setPen(gridLineColor);

    for(int row=0; row<ROW_COUNT + 1; row++)
    {
        QPoint start(0, row*CELL_HEIGHT);
        QPoint end(wantedWidth, row*CELL_HEIGHT);
        painter.drawLine(start, end);
    }
    for(int col=0; col<COL_COUNT + 1; col++)
    {
        QPoint start(FIRST_COL_WIDTH + col*CELL_WIDTH, 0);
        QPoint end(FIRST_COL_WIDTH + col*CELL_WIDTH, wantedHeight);
        painter.drawLine(start, end);
    }

    // Draw the highlight for the selected one
    if(m_selectedAddress>-1)
    {
        int col = m_selectedAddress % 32;
        int row = m_selectedAddress / 32;
        QRect textRect(FIRST_COL_WIDTH + col*CELL_WIDTH, (row+1)*CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT);
        painter.setPen(QColor("black"));
        painter.drawRect(textRect);
    }
    qDebug() << "Painter took " << t.elapsed();
}

int UniverseDisplay::cellHitTest(const QPoint &point)
{
    qreal wantedHeight = CELL_HEIGHT * (ROW_COUNT + 1);
    qreal wantedWidth = FIRST_COL_WIDTH + CELL_WIDTH * COL_COUNT;

    qreal scaleWidth = width()/wantedWidth;
    qreal scaleHeight = height()/ wantedHeight;
    qreal minScale = qMin(scaleWidth, scaleHeight);

    QRectF drawnRect(0, 0, wantedWidth*minScale, wantedHeight*minScale);
    drawnRect.moveCenter(this->rect().center());
    drawnRect.moveTop(0);

    if(!drawnRect.contains(point))
        return -1;


    qreal cellWidth = (drawnRect.width() - FIRST_COL_WIDTH * minScale) / COL_COUNT;
    qreal firstColWidth = drawnRect.width() - (COL_COUNT * cellWidth);
    qreal cellHeight = drawnRect.height() / (ROW_COUNT + 1);

    int col = (point.x() - drawnRect.left() - firstColWidth) / cellWidth;
    int row = (point.y() - drawnRect.top() - cellHeight) / cellHeight;

    if(col<0)  return -1;
    if(row<0)  return -1;

    int address = (row * 32) + col;
    if(address>511 || address<0) return -1;
    return address;

}

void UniverseDisplay::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);

    if(event->buttons() & Qt::LeftButton)
    {
        int address = cellHitTest(event->pos());
        if(m_selectedAddress!=address)
        {
            m_selectedAddress = address;
            emit selectedAddressChanged(m_selectedAddress);
            update();
        }
    }

}

void UniverseDisplay::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    if(event->buttons() & Qt::LeftButton)
    {
        int address = cellHitTest(event->pos());
        if(m_selectedAddress!=address)
        {
            m_selectedAddress = address;
            emit selectedAddressChanged(m_selectedAddress);
            update();
        }
    }
}