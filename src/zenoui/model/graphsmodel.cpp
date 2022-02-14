#include "subgraphmodel.h"
#include "graphsmodel.h"
#include "modelrole.h"
#include <zenoui/util/uihelper.h>
#include <zeno/zeno.h>


GraphsModel::GraphsModel(QObject *parent)
    : IGraphsModel(parent)
    , m_selection(nullptr)
    , m_dirty(false)
{
    m_selection = new QItemSelectionModel(this);
}

GraphsModel::~GraphsModel()
{
}

QItemSelectionModel* GraphsModel::selectionModel() const
{
    return m_selection;
}

void GraphsModel::setFilePath(const QString& fn)
{
    m_filePath = fn;
}

SubGraphModel* GraphsModel::subGraph(const QString& name) const
{
    for (int i = 0; i < m_subGraphs.size(); i++)
    {
        if (m_subGraphs[i]->name() == name)
            return m_subGraphs[i];
    }
    return nullptr;

    const QModelIndexList& lst = this->match(index(0, 0), ROLE_OBJNAME, name, 1, Qt::MatchExactly);
    if (lst.size() > 0)
    {
        SubGraphModel* pModel = static_cast<SubGraphModel*>(lst[0].data(ROLE_GRAPHPTR).value<void*>());
        return pModel;
    }
    return nullptr;
}

SubGraphModel* GraphsModel::subGraph(int idx) const
{
    if (idx >= 0 && idx < m_subGraphs.count())
    {
        return m_subGraphs[idx];
    }
    return nullptr;

    QModelIndex index = this->index(idx, 0);
    if (index.isValid())
    {
        SubGraphModel *pModel = static_cast<SubGraphModel *>(index.data(ROLE_GRAPHPTR).value<void *>());
        return pModel;
    }
    return nullptr;
}

SubGraphModel* GraphsModel::currentGraph()
{
    return subGraph(m_selection->currentIndex().row());
}

void GraphsModel::switchSubGraph(const QString& graphName)
{
    QModelIndex startIndex = createIndex(0, 0, nullptr);
    const QModelIndexList &lst = this->match(startIndex, ROLE_OBJNAME, graphName, 1, Qt::MatchExactly);
    if (lst.size() == 1)
    {
        m_selection->setCurrentIndex(lst[0], QItemSelectionModel::Current);
    }
}

void GraphsModel::newSubgraph(const QString &graphName)
{
    QModelIndex startIndex = createIndex(0, 0, nullptr);
    const QModelIndexList &lst = this->match(startIndex, ROLE_OBJNAME, graphName, 1, Qt::MatchExactly);
    if (lst.size() == 1)
    {
        m_selection->setCurrentIndex(lst[0], QItemSelectionModel::Current);
    }
    else
    {
        SubGraphModel *subGraphModel = new SubGraphModel(this);
        subGraphModel->setName(graphName);
        appendSubGraph(subGraphModel);
        m_selection->setCurrentIndex(index(rowCount() - 1, 0), QItemSelectionModel::Current);
        markDirty();
    }
}

void GraphsModel::reloadSubGraph(const QString& graphName)
{
    initDescriptors();
    SubGraphModel *pReloadModel = subGraph(graphName);
    Q_ASSERT(pReloadModel);
    NODES_DATA datas = pReloadModel->nodes();

    pReloadModel->clear();

    pReloadModel->blockSignals(true);
    for (auto data : datas)
    {
        pReloadModel->appendItem(data);
    }
    pReloadModel->blockSignals(false);
    pReloadModel->reload();
    markDirty();
}

void GraphsModel::renameSubGraph(const QString& oldName, const QString& newName)
{
    //todo: transaction.
    SubGraphModel* pSubModel = subGraph(oldName);
    Q_ASSERT(pSubModel);
    pSubModel->setName(newName);

    for (int r = 0; r < this->rowCount(); r++)
    {
        QModelIndex index = this->index(r, 0);
        Q_ASSERT(index.isValid());
        SubGraphModel* pModel = static_cast<SubGraphModel*>(index.data(ROLE_GRAPHPTR).value<void*>());
        const QString& subgraphName = pModel->name();
        if (subgraphName != oldName)
        {
            pModel->replaceSubGraphNode(oldName, newName);
        }
    }
    emit graphRenamed(oldName, newName);
}

QModelIndex GraphsModel::index(int row, int column, const QModelIndex& parent) const
{
    if (row < 0 || row >= m_subGraphs.size())
        return QModelIndex();

    return createIndex(row, 0, nullptr);
    //return _base::index(row, column, parent);
}

QModelIndex GraphsModel::index(const QString& subGraphName) const
{
	for (int row = 0; row < m_subGraphs.size(); row++)
	{
		if (m_subGraphs[row]->name() == subGraphName)
		{
            return createIndex(row, 0, nullptr);
		}
	}
    return QModelIndex();
}

QModelIndex GraphsModel::indexBySubModel(SubGraphModel* pSubModel) const
{
    int row = m_subGraphs.indexOf(pSubModel);
    return createIndex(row, 0, nullptr);
}

QModelIndex GraphsModel::parent(const QModelIndex& child) const
{
    return QModelIndex();
}

QVariant GraphsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::DisplayRole)
    {
        return m_subGraphs[index.row()]->name();
    }
    return QVariant();
}

int GraphsModel::rowCount(const QModelIndex& parent) const
{
    return m_subGraphs.size();
}

int GraphsModel::columnCount(const QModelIndex& parent) const
{
    return 1;
}

bool GraphsModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role == Qt::EditRole)
    {
        QString oldName = index.data(ROLE_OBJNAME).toString();
        if (oldName != value.toString())
        {
            renameSubGraph(oldName, value.toString());
            _base::setData(index, value, ROLE_OBJNAME);
        }
    }
    return false;
}

void GraphsModel::initDescriptors()
{
    NODE_DESCS descs;
    QString strDescs = QString::fromStdString(zeno::dumpDescriptors());
    QStringList L = strDescs.split("\n");
    for (int i = 0; i < L.size(); i++)
    {
        QString line = L[i];
        if (line.startsWith("DESC@"))
        {
            line = line.trimmed();
            int idx1 = line.indexOf("@");
            int idx2 = line.indexOf("@", idx1 + 1);
            Q_ASSERT(idx1 != -1 && idx2 != -1);
            QString wtf = line.mid(0, idx1);
            QString z_name = line.mid(idx1 + 1, idx2 - idx1 - 1);
            QString rest = line.mid(idx2 + 1);
            Q_ASSERT(rest.startsWith("{") && rest.endsWith("}"));
            auto _L = rest.mid(1, rest.length() - 2).split("}{");
            QString inputs = _L[0], outputs = _L[1], params = _L[2], categories = _L[3];
            QStringList z_categories = categories.split('%', Qt::SkipEmptyParts);
            QJsonArray z_inputs;

            NODE_DESC desc;
            for (QString input : inputs.split("%", Qt::SkipEmptyParts))
            {
                QString type, name, defl;
                auto _arr = input.split('@');
                Q_ASSERT(_arr.size() == 3);
                type = _arr[0];
                name = _arr[1];
                defl = _arr[2];
                INPUT_SOCKET socket;
                socket.info.type = type;
                socket.info.name = name;
                socket.info.defaultValue = UiHelper::_parseDefaultValue(defl, type);
                desc.inputs[name] = socket;
            }
            for (QString output : outputs.split("%", Qt::SkipEmptyParts))
            {
                QString type, name, defl;
				auto _arr = output.split('@');
				Q_ASSERT(_arr.size() == 3);
				type = _arr[0];
				name = _arr[1];
				defl = _arr[2];
                OUTPUT_SOCKET socket;
				socket.info.type = type;
				socket.info.name = name;
				socket.info.defaultValue = UiHelper::_parseDefaultValue(defl, type);
                desc.outputs[name] = socket;
            }
            for (QString param : params.split("%", Qt::SkipEmptyParts))
            {
                QString type, name, defl;
                auto _arr = param.split('@');
				type = _arr[0];
				name = _arr[1];
				defl = _arr[2];
                PARAM_INFO paramInfo;
                paramInfo.name = name;
                paramInfo.typeDesc = type;
                paramInfo.defaultValue = UiHelper::_parseDefaultValue(defl, type);
                desc.params[name] = paramInfo;
            }
            desc.categories = z_categories;

            if (z_name == "SubInput")
            {
                int j;
                j = 0;
            }

            descs.insert(z_name, desc);
        }
    }
    setDescriptors(descs);
}

NODE_DESCS GraphsModel::getSubgraphDescs()
{
    NODE_DESCS descs;
    for (int r = 0; r < this->rowCount(); r++)
    {
        QModelIndex index = this->index(r, 0);
        Q_ASSERT(index.isValid());
        SubGraphModel *pModel = static_cast<SubGraphModel *>(index.data(ROLE_GRAPHPTR).value<void *>());
        const QString& graphName = pModel->name();
        if (graphName == "main")
            continue;

        QString subcategory = "subgraph";
        INPUT_SOCKETS subInputs;
        OUTPUT_SOCKETS subOutputs;
        for (int i = 0; i < pModel->rowCount(); i++)
        {
            QModelIndex idx = pModel->index(i, 0);
            const QString& nodeName = idx.data(ROLE_OBJNAME).toString();
            PARAMS_INFO params = idx.data(ROLE_PARAMETERS).value<PARAMS_INFO>();
            if (nodeName == "SubInput")
            {
                QString n_type = params["type"].value.toString();
                QString n_name = params["name"].value.toString();
                auto n_defl = params["defl"].value;

                SOCKET_INFO info;
                info.name = n_name;
                info.type = n_type;
                info.defaultValue = n_defl; 

                INPUT_SOCKET inputSock;
                inputSock.info = info;
                subInputs.insert(n_name, inputSock);
            }
            else if (nodeName == "SubOutput")
            {
                auto n_type = params["type"].value.toString();
                auto n_name = params["name"].value.toString();
                auto n_defl = params["defl"].value;

                SOCKET_INFO info;
                info.name = n_name;
                info.type = n_type;
                info.defaultValue = n_defl; 

                OUTPUT_SOCKET outputSock;
                outputSock.info = info;
                subOutputs.insert(n_name, outputSock);
            }
            else if (nodeName == "SubCategory")
            {
                subcategory = params["name"].value.toString();
            }
        }

        subInputs.insert(m_nodesDesc["Subgraph"].inputs);
        subOutputs.insert(m_nodesDesc["Subgraph"].outputs);
        
        NODE_DESC desc;
        desc.inputs = subInputs;
        desc.outputs = subOutputs;
        desc.categories.push_back(subcategory);
        desc.is_subgraph = true;
        descs[graphName] = desc;
    }
    return descs;
}

NODE_DESCS GraphsModel::descriptors() const
{
    return m_nodesDesc;
}

void GraphsModel::setDescriptors(const NODE_DESCS& nodeDescs)
{
    m_nodesDesc = nodeDescs;
    m_nodesCate.clear();
    for (auto it = m_nodesDesc.constBegin(); it != m_nodesDesc.constEnd(); it++)
    {
        const QString& name = it.key();
        const NODE_DESC& desc = it.value();
        for (auto cate : desc.categories)
        {
            m_nodesCate[cate].name = cate;
            m_nodesCate[cate].nodes.push_back(name);
        }
    }
}

void GraphsModel::appendSubGraph(SubGraphModel* pGraph)
{
    int row = m_subGraphs.size();
	beginInsertRows(QModelIndex(), row, row);
    m_subGraphs.append(pGraph);
	endInsertRows();
    return;

    QStandardItem *pItem = new QStandardItem;
    QString graphName = pGraph->name();
    QVariant var(QVariant::fromValue(static_cast<void *>(pGraph)));
    pItem->setText(graphName);
    pItem->setData(var, ROLE_GRAPHPTR);
    pItem->setData(graphName, ROLE_OBJNAME);
    //appendRow(pItem);
}

void GraphsModel::removeGraph(int idx)
{
    beginRemoveRows(QModelIndex(), idx, idx);
    m_subGraphs.remove(idx);
    endRemoveRows();
    markDirty();
    //removeRow(idx);
    //markDirty();
}

NODE_CATES GraphsModel::getCates()
{
    return m_nodesCate;
}

QString GraphsModel::filePath() const
{
    return m_filePath;
}

QString GraphsModel::fileName() const
{
    QFileInfo fi(m_filePath);
    Q_ASSERT(fi.isFile());
    return fi.fileName();
}

void GraphsModel::onCurrentIndexChanged(int row)
{
    const QString& graphName = data(index(row, 0), ROLE_OBJNAME).toString();
    switchSubGraph(graphName);
}

bool GraphsModel::isDirty() const
{
    return m_dirty;
}

void GraphsModel::markDirty()
{
    m_dirty = true;
}

void GraphsModel::clearDirty()
{
    m_dirty = false;
}

void GraphsModel::onRemoveCurrentItem()
{
    removeGraph(m_selection->currentIndex().row());
    //switch to main.
    QModelIndex startIndex = createIndex(0, 0, nullptr);
    //to ask: if not main scene?
    const QModelIndexList &lst = this->match(startIndex, ROLE_OBJNAME, "main", 1, Qt::MatchExactly);
    if (lst.size() == 1)
        m_selection->setCurrentIndex(index(lst[0].row(), 0), QItemSelectionModel::Current);
}


void GraphsModel::beginTransaction(const QModelIndex& subGpIdx)
{
    SubGraphModel* pGraph = subGraph(subGpIdx.row());
    Q_ASSERT(pGraph);
    if (pGraph)
    {

    }
}

void GraphsModel::endTransaction(const QModelIndex& subGpIdx)
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
    Q_ASSERT(pGraph);
	if (pGraph)
	{

	}
}

QModelIndex GraphsModel::index(const QString& id, const QModelIndex& subGpIdx)
{
    SubGraphModel* pGraph = subGraph(subGpIdx.row());
    Q_ASSERT(pGraph);
    if (pGraph)
    {
        // index of SubGraph rather than Graphs.
        return pGraph->index(id);
    }
    return QModelIndex();
}

QModelIndex GraphsModel::index(int r, const QModelIndex& subGpIdx)
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
	if (pGraph)
	{
		// index of SubGraph rather than Graphs.
		return pGraph->index(r, 0);
	}
	return QModelIndex();
}

QVariant GraphsModel::data2(const QModelIndex& subGpIdx, const QModelIndex& index, int role)
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
    if (pGraph)
    {
        return pGraph->data(index, role);
    }
    return QVariant();
}

void GraphsModel::setData2(const QModelIndex& subGpIdx, const QModelIndex& index, const QVariant& value, int role)
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
    if (pGraph)
    {
        pGraph->setData(index, value, role);
    }
}

int GraphsModel::itemCount(const QModelIndex& subGpIdx) const
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
    if (pGraph)
    {
        return pGraph->rowCount();
    }
    return 0;
}

void GraphsModel::appendItem(const NODE_DATA& nodeData, const QModelIndex& subGpIdx)
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
    if (pGraph)
    {
        pGraph->appendItem(nodeData);
    }
}

void GraphsModel::appendNodes(const QList<NODE_DATA>& nodes, const QModelIndex& subGpIdx)
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
    if (pGraph)
    {
        pGraph->appendNodes(nodes);
    }
}

void GraphsModel::removeNode(const QString& nodeid, const QModelIndex& subGpIdx)
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
    if (pGraph)
    {
        //todo: transaction.
        QModelIndex idx = pGraph->index(nodeid);
        pGraph->removeNode(nodeid, false);
    }
}

void GraphsModel::removeNode(int row, const QModelIndex& subGpIdx)
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
	if (pGraph)
	{
        QModelIndex idx = pGraph->index(row, 0);
        pGraph->removeNode(row);
	}
}

void GraphsModel::removeLink(const EdgeInfo& info, const QModelIndex& subGpIdx)
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
	if (pGraph)
	{
        pGraph->removeLink(info);
	}
}

void GraphsModel::removeSubGraph(const QString& name)
{
	for (int i = 0; i < m_subGraphs.size(); i++)
	{
        if (m_subGraphs[i]->name() == name)
        {
            removeGraph(i);
            return;
        }
	}
}

void GraphsModel::addLink(const EdgeInfo& info, const QModelIndex& subGpIdx)
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
    if (pGraph)
    {
        pGraph->addLink(info);
    }
}

void GraphsModel::updateParamInfo(const QString& id, PARAM_UPDATE_INFO info, const QModelIndex& subGpIdx)
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
    if (pGraph)
    {
        pGraph->updateParam(id, info.name, info.newValue);
    }
}

void GraphsModel::updateSocket(const QString& id, SOCKET_UPDATE_INFO info, const QModelIndex& subGpIdx)
{
	SubGraphModel* pSubg = subGraph(subGpIdx.row());
	Q_ASSERT(pSubg);
    if (pSubg)
    {
        pSubg->updateSocket(id, info);
    }
}

NODE_DATA GraphsModel::itemData(const QModelIndex& index, const QModelIndex& subGpIdx) const
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
    if (pGraph)
    {
        return pGraph->itemData(index);
    }
    return NODE_DATA();
}

QString GraphsModel::name(const QModelIndex& subGpIdx) const
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
    if (pGraph)
    {
        return pGraph->name();
    }
    return "";
}

void GraphsModel::setName(const QString& name, const QModelIndex& subGpIdx)
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
    if (pGraph)
    {
        pGraph->setName(name);
    }
}

void GraphsModel::replaceSubGraphNode(const QString& oldName, const QString& newName, const QModelIndex& subGpIdx)
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
    if (pGraph)
    {
        pGraph->replaceSubGraphNode(oldName, newName);
    }
}

NODES_DATA GraphsModel::nodes(const QModelIndex& subGpIdx)
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
    if (pGraph)
    {
        return pGraph->nodes();
    }
    return NODES_DATA();
}

void GraphsModel::clear(const QModelIndex& subGpIdx)
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
    if (pGraph)
    {
        pGraph->clear();
    }
}

void GraphsModel::reload(const QModelIndex& subGpIdx)
{
	SubGraphModel* pGraph = subGraph(subGpIdx.row());
	Q_ASSERT(pGraph);
    if (pGraph)
    {
        //todo
        pGraph->reload();
    }
}

void GraphsModel::onModelInited()
{

}

void GraphsModel::undo()
{
    //todo
}

void GraphsModel::redo()
{
    //todo
}

QModelIndexList GraphsModel::searchInSubgraph(const QString& objName, const QModelIndex& subgIdx)
{
    SubGraphModel* pModel = subGraph(subgIdx.row());
    return pModel->match(pModel->index(0, 0), ROLE_OBJNAME, objName, -1, Qt::MatchContains);
}


void GraphsModel::on_dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    SubGraphModel* pSubModel = qobject_cast<SubGraphModel*>(sender());
    Q_ASSERT(pSubModel && roles.size() == 1);
    QModelIndex subgIdx = indexBySubModel(pSubModel);
    emit _dataChanged(subgIdx, topLeft, roles[0]);
}

void GraphsModel::on_rowsAboutToBeInserted(const QModelIndex& parent, int first, int last)
{
    SubGraphModel* pSubModel = qobject_cast<SubGraphModel*>(sender());
    Q_ASSERT(pSubModel);
    QModelIndex subgIdx = indexBySubModel(pSubModel);
    emit _rowsAboutToBeInserted(subgIdx, first, last);
}

void GraphsModel::on_rowsInserted(const QModelIndex& parent, int first, int last)
{
    SubGraphModel* pSubModel = qobject_cast<SubGraphModel*>(sender());
    Q_ASSERT(pSubModel);
    QModelIndex subgIdx = indexBySubModel(pSubModel);
    emit _rowsInserted(subgIdx, parent, first, last);
}

void GraphsModel::on_rowsAboutToBeRemoved(const QModelIndex& parent, int first, int last)
{
    SubGraphModel* pSubModel = qobject_cast<SubGraphModel*>(sender());
    Q_ASSERT(pSubModel);
    QModelIndex subgIdx = indexBySubModel(pSubModel);
    emit _rowsAboutToBeRemoved(subgIdx, parent, first, last);
}

void GraphsModel::on_rowsRemoved(const QModelIndex& parent, int first, int last)
{
    SubGraphModel* pSubModel = qobject_cast<SubGraphModel*>(sender());
    Q_ASSERT(pSubModel);
    QModelIndex subgIdx = indexBySubModel(pSubModel);
    emit _rowsRemoved(parent, first, last);
}