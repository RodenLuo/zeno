#include "zsgwriter.h"


ZsgWriter::ZsgWriter()
{
}

ZsgWriter& ZsgWriter::getInstance()
{
    static ZsgWriter writer;
    return writer;
}

QString ZsgWriter::dumpProgram(GraphsModel* pModel)
{
    QJsonObject obj = dumpGraphs(pModel);
    QJsonDocument doc(obj);
    QString strJson(doc.toJson(QJsonDocument::Compact));
    return strJson;
}

QString ZsgWriter::dumpSubGraph(SubGraphModel* pSubModel)
{
    QJsonObject obj = _dumpSubGraph(pSubModel);
    QJsonDocument doc(obj);
    QString strJson(doc.toJson(QJsonDocument::Compact));
    return strJson;
}

QJsonObject ZsgWriter::dumpGraphs(GraphsModel* pModel)
{
    QJsonObject obj;
    if (!pModel)
        return obj;

    QJsonObject graphObj;
    for (int i = 0; i < pModel->rowCount(); i++)
    {
        SubGraphModel* pSubModel = pModel->subGraph(i);
        graphObj.insert(pSubModel->name(), _dumpSubGraph(pSubModel));
    }
    obj.insert("graph", graphObj);
    obj.insert("views", QJsonObject());     //todo

    NODE_DESCS descs = pModel->descriptors();
    obj.insert("descs", _dumpDescriptors(descs));

    return obj;
}

QJsonObject ZsgWriter::_dumpSubGraph(SubGraphModel *pSubModel)
{
    QJsonObject obj;
    if (!pSubModel)
        return obj;

    QJsonObject nodesObj;
    const NODES_DATA &nodes = pSubModel->nodes();
    for (const NODE_DATA& node : nodes)
    {
        const QString& id = node[ROLE_OBJID].toString();
        nodesObj.insert(id, dumpNode(node));
    }
    obj.insert("nodes", nodesObj);

    QRectF viewRc = pSubModel->viewRect();
    if (!viewRc.isNull())
    {
        QJsonObject rcObj;
        rcObj.insert("x", viewRc.x());
        rcObj.insert("y", viewRc.y());
        rcObj.insert("width", viewRc.width());
        rcObj.insert("height", viewRc.height());
        obj.insert("view_rect", rcObj);
    }
    return obj;
}

QJsonObject ZsgWriter::dumpNode(const NODE_DATA& data)
{
    QJsonObject obj;
    obj.insert("name", data[ROLE_OBJNAME].toString());
    
    const INPUT_SOCKETS &inputs = data[ROLE_INPUTS].value<INPUT_SOCKETS>();
    QJsonObject inputsArr;
    for (const INPUT_SOCKET& inSock : inputs)
    {
        if (!inSock.outNodes.isEmpty())
        {
            for (const SOCKETS_INFO &outSocks : inSock.outNodes) {
                for (const SOCKET_INFO &outSock : outSocks) {
                    QJsonArray arr;
                    arr.push_back(outSock.nodeid);
                    arr.push_back(outSock.name);
                    const QVariant &defl = outSock.defaultValue;
                    if (defl.type() == QVariant::String)
                        arr.push_back(defl.toString());
                    else if (defl.type() == QVariant::Double)
                        arr.push_back(defl.toFloat());
                    else
                        arr.push_back(QJsonValue::Null);
                    inputsArr.insert(inSock.info.name, arr);
                }
            }
        }
        else
        {
            QJsonArray arr = {QJsonValue::Null, QJsonValue::Null, QJsonValue::Null};
            inputsArr.insert(inSock.info.name, arr);
        }
    }
    obj.insert("inputs", inputsArr);

    const PARAMS_INFO& params = data[ROLE_PARAMETERS].value<PARAMS_INFO>();
    QJsonObject paramsObj;
    for (const PARAM_INFO& info : params)
    {
        QVariant val = info.value;
        if (val.type() == QVariant::String)
            paramsObj.insert(info.name, val.toString());
        else if (val.type() == QVariant::Double)
            paramsObj.insert(info.name, val.toFloat());
        else if (val.type() == QVariant::Int)
            paramsObj.insert(info.name, val.toInt());
    }
    obj.insert("params", paramsObj);

    QPointF pos = data[ROLE_OBJPOS].toPointF();
    obj.insert("uipos", QJsonArray({pos.x(), pos.y()}));

    QJsonObject options;
    //todo: option
    obj.insert("options", options);

    return obj;
}

QJsonObject ZsgWriter::_dumpDescriptors(const NODE_DESCS& descs)
{
    QJsonObject descsObj;
    for (auto name : descs.keys())
    {
        QJsonObject descObj;
        const NODE_DESC& desc = descs[name];

        if (name == "SDFToPoly")
        {
            int j;
            j = 0;
        }

        QJsonArray inputs;
        for (INPUT_SOCKET inSock : desc.inputs)
        {
            QJsonArray socketInfo;
            socketInfo.push_back(inSock.info.type);
            socketInfo.push_back(inSock.info.name);

            const QVariant &defl = inSock.info.defaultValue;
            if (defl.type() == QVariant::String)
                socketInfo.push_back(defl.toString());
            else if (defl.type() == QVariant::Double)
                socketInfo.push_back(defl.toFloat());
            else
                socketInfo.push_back("");

            inputs.push_back(socketInfo);
        }

        QJsonArray params;
        for (PARAM_INFO param : desc.params)
        {
            QJsonArray paramInfo;
            paramInfo.push_back(param.typeDesc);
            paramInfo.push_back(param.name);

            const QVariant &defl = param.defaultValue;
            if (defl.type() == QVariant::String)
                paramInfo.push_back(defl.toString());
            else if (defl.type() == QVariant::Double)
                paramInfo.push_back(defl.toFloat());
            else
                paramInfo.push_back("");

            params.push_back(paramInfo);
        }

        QJsonArray outputs;
        for (OUTPUT_SOCKET outSock : desc.outputs)
        {
            QJsonArray socketInfo;
            socketInfo.push_back(outSock.info.type);
            socketInfo.push_back(outSock.info.name);

            const QVariant &defl = outSock.info.defaultValue;
            if (defl.type() == QVariant::String)
                socketInfo.push_back(defl.toString());
            else if (defl.type() == QVariant::Double)
                socketInfo.push_back(defl.toFloat());
            else
                socketInfo.push_back("");

            outputs.push_back(socketInfo);
        }

        QJsonArray categories;
        for (QString cate : desc.categories)
        {
            categories.push_back(cate);
        }

        descObj.insert("inputs", inputs);
        descObj.insert("outputs", outputs);
        descObj.insert("params", params);
        descObj.insert("categories", categories);
        if (desc.is_subgraph)
            descObj.insert("is_subgraph", desc.is_subgraph);

        descsObj.insert(name, descObj);
    }
    return descsObj;
}