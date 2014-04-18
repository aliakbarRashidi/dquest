/**
	@author Ben Lau
 */

#include <QSqlError>
#include "dqmodel.h"
#include "priv/dqsqlitestatement.h"
#include "priv/dqsqliteengine.h"

DQSqliteEngine::DQSqliteEngine() : m_sql(new DQSqliteStatement())
{
}

DQSqliteEngine::~DQSqliteEngine(){

}

QString DQSqliteEngine::name(){
    return "SQLITE";
}

bool DQSqliteEngine::open(QSqlDatabase  db) {
    Q_ASSERT(db.isOpen());

    if (db.driverName() != "QSQLITE") {
        qWarning() << "Only QSQLITE dirver is supported.";
        return false;
    }

    m_sql.setDatabase(db);
    return true;
}

bool DQSqliteEngine::isOpen() const{
    return m_sql.database().isOpen();
}

void DQSqliteEngine::close(){
    m_sql.setDatabase(QSqlDatabase());
}

/// Add a model to the engine
bool DQSqliteEngine::addModel(DQModelMetaInfo* info) {
    bool res = false;
    if (!info) {
        return res;
    }

    if (!m_models.contains(info)) {
        m_models.append(info);
        res = true;
    }

    return res;
}

QList<DQModelMetaInfo*> DQSqliteEngine::modelList() const{
    return m_models;
}

bool DQSqliteEngine::createModel(DQModelMetaInfo* info){
    bool res = true;

    if (!m_sql.exists(info)) {

        if (!m_sql.createTableIfNotExists(info)){
            qWarning() << QString("DQConnection::createTables() - Failed to create table for %1 . Error : %2").arg(info->className())
                    .arg( m_sql.lastQuery().lastError().text());
            qWarning() << m_sql.lastQuery().lastQuery();
            res = false;
        } else {

            DQSharedList initialData = info->initialData();
            /// TODO: Transaction
            int n = initialData.size();
            for (int i = 0 ; i< n;i++) {
                if (!update(initialData.at(i))){
                    qWarning() << "Failed to save: " <<initialData.at(i);
                }
            }
        }
        setLastQuery( m_sql.lastQuery() );
    }


    return res;
}

bool DQSqliteEngine::dropModel(DQModelMetaInfo* info){
    bool res = true;
    if (!m_sql.exists(info))
        return true;

    if (!m_sql.dropTable(info)) {
        res = false;
    }

    setLastQuery(m_sql.lastQuery());

    return res;
}

bool DQSqliteEngine::existsModel(DQModelMetaInfo* info){
    return m_sql.exists(info);
}

/// Update the database with record.
/**
 *  @param fields The changed fields. If it is omitted, it will assume all the field should be updated.
 *  @param forceInsert TRUE if the data should be inserted to the database as a new record regardless of the original id. The id field will be updated after operation.
 *
 *  @remarks The behaviour is depended on the engine itself.
 */
bool DQSqliteEngine::update(DQAbstractModel* model, QStringList fields, bool forceInsert){

    DQModelMetaInfo *info = model->metaInfo();
    bool res;

    if (fields.isEmpty()) {
        fields = info->fieldNameList();
    }

    QVariant id; // The ID of the record.

    id = info->value(model,"id"); // @TODO Change to primary key

    if (forceInsert || id.isNull()) {
        res = m_sql.replaceInto(info,model,fields,true);
    } else {
        res = m_sql.replaceInto(info,model,fields,false);
    }
    return res;
}

/// Create index
bool DQSqliteEngine::createIndex(const DQBaseIndex &index){
    return m_sql.createIndexIfNotExists(index);
}

/// Drop the index
bool DQSqliteEngine::dropIndex(QString name){
    return m_sql.dropIndexIfExists(name);
}

/// Get the assoicated DQSql instance
DQSql& DQSqliteEngine::sql(){
    return m_sql;
}

void DQSqliteEngine::setLastQuery(QSqlQuery query){
    if (!isOpen())
        return;

    mutex.lock();
    m_lastQuery = query;
    mutex.unlock();
}

QSqlQuery DQSqliteEngine::query(){
    return m_sql.query();
}

QSqlQuery DQSqliteEngine::lastQuery(){
    return m_lastQuery;
}
