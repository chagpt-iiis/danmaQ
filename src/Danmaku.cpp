﻿/*
 * This file is part of danmaQ.
 * 
 * DanmaQ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DanmaQ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Danmaku.hpp"
#include <QtCore>
#include <QApplication>
#include <QDesktopWidget>
#include <QLabel>
#include <QWidget>
#include <QColor>
#include <QGraphicsDropShadowEffect>
#include <QAbstractAnimation>
#include <QPropertyAnimation>
#include <QRect>
#include <QTimer>
#include <QDebug>
#include <QString>

#include <cmath>
#include <map>
#include <utility>

#if defined _WIN32 || defined __CYGWIN__
#include <Windows.h>
#endif

#include "common.hpp"

// static std::map<QString, std::pair<QString, QColor>> colormap = {
// 	{"white", std::make_pair("rgb(255, 255, 255)", QColor("black"))},
// 	{"black", std::make_pair("rgb(0, 0, 0)", QColor("white"))},
// 	{"blue", std::make_pair("rgb(20, 95, 198)", QColor("white"))},
// 	{"cyan", std::make_pair("rgb(0, 255, 255)", QColor("black"))},
// 	{"red", std::make_pair("rgb(231, 34, 0)", QColor("white"))},
// 	{"yellow", std::make_pair("rgb(255, 221, 2)", QColor("black"))},
// 	{"green", std::make_pair("rgb(4, 202, 0)", QColor("black"))},
// 	{"purple", std::make_pair("rgb(128, 0, 128)", QColor("white"))}
// };

// Danmaku::Danmaku(QString text, QWidget *parent): Danmaku(text, "blue", FLY, -1, parent){};

Danmaku::Danmaku(QString text, int color, Position position, int slot, DMCanvas *parent, DMMainWindow *mainWindow)
	: QLabel(text, parent)
{
	this->canvas = parent;
	this->mainWindow = mainWindow;
	this->setAttribute(Qt::WA_DeleteOnClose);
	this->setTextFormat(Qt::PlainText);

	int r = (color >> 24) & 255, g = (color >> 16) & 255, b = (color >> 8) & 255, a = color & 255;

	QString tcolor = QString("rgba(%1, %2, %3, %4%)").arg(r).arg(g).arg(b).arg(a / 2.55);
	QColor bcolor = r + g + b >= 384 ? QColor(0, 0, 0) : QColor(255, 255, 255);

	QString style = style_tmpl
						.arg(this->mainWindow->fontSize)
						.arg(this->mainWindow->fontFamily)
						.arg(tcolor);

	QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);

	bool enableShadow = false;

	this->setWindowFlags(
		Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint | Qt::ToolTip | Qt::FramelessWindowHint);
	this->setAttribute(Qt::WA_TranslucentBackground, true);
	this->setAttribute(Qt::WA_DeleteOnClose, true);
	this->setAttribute(Qt::WA_Disabled, true);
	this->setAttribute(Qt::WA_TransparentForMouseEvents, true);
	this->setStyleSheet("background: transparent");

	enableShadow = true;

	if (enableShadow)
	{
#if defined _WIN32 || defined __CYGWIN__
		// remove CS_DROPSHADOW
		SetClassLong((HWND)this->winId(), -26, 0x0008 & ~0x00020000);
#endif
		effect->setBlurRadius(6);
		effect->setColor(bcolor);
		effect->setOffset(0, 0);
		this->setGraphicsEffect(effect);
	}

	this->setStyleSheet(style);
	this->setContentsMargins(0, 0, 0, 0);

	QSize _msize = this->minimumSizeHint();
	this->resize(_msize);

	this->position = position;
	this->slot = slot;
	this->init_position();
}

QString Danmaku::style_tmpl = QString(
	"font-size: %1pt;"
	"font-weight: bold;"
	"font-family: %2;"
	"color: %3;");

void Danmaku::init_position(bool initial)
{
	int startX, startY, endX, endY;
	startY = endY = this->canvas->slot_y(this->slot);

	switch (this->position)
	{
	case topScrolling:
	case bottomScrolling:
		startX = this->canvas->screen.width() - 1;
		endX = -this->width();
		this->linearMotion(startX, startY, endX, endY);
		break;
	case topReverse:
		startX = -this->width();
		endX = this->canvas->screen.width() - 1;
		this->linearMotion(startX, startY, endX, endY);
		break;
	case topStatic:
	case bottomStatic:
		startX = endX = (this->canvas->screen.width() - this->width()) / 2;
		this->linearMotion(startX, startY, endX, endY);
		break;
	case vertical: {
		int x = 150;
		this->setFixedWidth(WIDTH_LIMIT);
		this->setWordWrap(true);
		this->adjustSize();
		QPoint p = this->canvas->getGlobalPoint(QPoint(x, startY - (this->height() - 18)));
		this->y = p.y();
		this->move(p);
		QTimer *timer = new QTimer(this);
		timer->setInterval(20 * 1000);
		connect(timer, &QTimer::timeout, this, &Danmaku::clean_close);
		timer->start();
		break;
	}
	default:
		break;
	}
}

void Danmaku::shift_up(int dy) {
	QRect cur = this->geometry();
	if ((this->y -= dy) < VMARGIN) {
		return this->clean_close();
	}
	QRect nxt = cur;
	nxt.moveTop(this->y);
	QPropertyAnimation *animation = new QPropertyAnimation(this, "geometry", this);
	animation->setDuration(100);
	animation->setStartValue(cur);
	animation->setEndValue(nxt);
	animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Danmaku::linearMotion(int startX, int startY, int endX, int endY)
{
	QPropertyAnimation *animation = new QPropertyAnimation(this, "geometry", this);
	QPoint start_point = this->canvas->getGlobalPoint(QPoint(startX, startY));
	QPoint end_point = this->canvas->getGlobalPoint(QPoint(endX, endY));
	animation->setDuration(10 * 1000);
	animation->setStartValue(
		QRect(start_point.x(), start_point.y(), this->width(), this->height()));
	animation->setEndValue(
		QRect(end_point.x(), end_point.y(), this->width(), this->height()));
	animation->start(QAbstractAnimation::DeleteWhenStopped);

	connect(
		animation, &QPropertyAnimation::finished,
		this, &Danmaku::clean_close);
}

void Danmaku::clean_close()
{
	this->close();
	emit exited(this);
}