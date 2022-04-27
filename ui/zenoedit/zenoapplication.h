#ifndef __ZENO_APPLICATION_H__
#define __ZENO_APPLICATION_H__

#include <QtWidgets>

class GraphsManagment;
class ZenoMainWindow;

class GraphsModel;

class ZenoApplication : public QApplication
{
	Q_OBJECT
public:
    ZenoApplication(int &argc, char **argv);
    ~ZenoApplication();
    QSharedPointer<GraphsManagment> graphsManagment() const;
    void initFonts();
    void initStyleSheets();
    void setIOProcessing(bool bIOProcessing);
    bool IsIOProcessing() const;
    ZenoMainWindow* getMainWindow();
    void setStatus(const QString& key, QVariant v);
    QVariant getStatus(const QString& key);

private:
    QString readQss(const QString& qssPath);

    QSharedPointer<GraphsManagment> m_pGraphs;
    bool m_bIOProcessing;
    QMap<QString, QVariant> m_global_status;
};

#define zenoApp (qobject_cast<ZenoApplication*>(QApplication::instance()))

#endif