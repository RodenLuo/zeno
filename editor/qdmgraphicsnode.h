#ifndef QDMGRAPHICSNODE_H
#define QDMGRAPHICSNODE_H

#include <QGraphicsItem>
#include <vector>
#include "qdmgraphicssocketin.h"
#include "qdmgraphicssocketout.h"
#include <QRectF>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>

class QDMGraphicsNode : public QGraphicsItem
{
    std::vector<QDMGraphicsSocketIn *> socketIns;
    std::vector<QDMGraphicsSocketOut *> socketOuts;

    QGraphicsTextItem *label;

public:
    QDMGraphicsNode();
    ~QDMGraphicsNode();

    void unlinkAll();
    float getHeight() const;
    virtual QRectF boundingRect() const override;
    virtual void paint(QPainter *painter, QStyleOptionGraphicsItem const *styleOptions, QWidget *widget) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    QDMGraphicsSocketIn *addSocketIn();
    QDMGraphicsSocketOut *addSocketOut();
    void setupByName(QString name);
    void setName(QString name);

    static constexpr float WIDTH = 200, HEIGHT = 60, ROUND = 6, BORDER = 3;
    static constexpr float SOCKMARGINTOP = 20, SOCKSTRIDE = 30, SOCKMARGINBOT = -10;
};

#endif // QDMGRAPHICSNODE_H
