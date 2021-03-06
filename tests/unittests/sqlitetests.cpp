#include "sqlitetests.h"

SqliteTests::SqliteTests(QObject* parent) : QObject(parent)
{
}

void SqliteTests::initTestCase()
{
    verifyCreateTable();
    foreignKey();

    DQConnection defaultConnection = DQConnection::defaultConnection();

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName( "tests.db" );

    QVERIFY( db.open() );

    QVERIFY( !defaultConnection.isOpen());
    QVERIFY (connect.open(db) );

    QVERIFY(defaultConnection.isOpen()); // connect become default connection

    DQSql sql = connect.sql();


    QVERIFY( sql.createTableIfNotExists<Model1>() );

    QVERIFY( sql.exists(dqMetaInfo<Model1>() ) );

    QVERIFY( sql.dropTable(dqMetaInfo<Model1>()) );

    QVERIFY( !sql.exists(dqMetaInfo<Model1>() ) );

    QVERIFY( sql.createTableIfNotExists<Model1>() );

    QVERIFY ( connect.addModel<Model1>() );
    QVERIFY ( connect.addModel<Model2>() );
    QVERIFY (!connect.addModel<Model1>() );
    QVERIFY (!connect.addModel<Model3>() );
    QVERIFY ( connect.addModel<Model4>() );
    QVERIFY ( connect.addModel<User>() );
    QVERIFY ( connect.addModel<Config>() );
    QVERIFY ( connect.addModel<ExamResult>() );
    QVERIFY ( connect.addModel<AllType>() );
    QVERIFY ( connect.addModel<HealthCheck>());

    QVERIFY( connect.dropTables() );

    QVERIFY( connect.createTables() ); // recreate table

    /* Create index */

    DQIndex<Model1> index1("index1");
    index1 << "key" << "value";

    QVERIFY(connect.createIndex(index1));

    // drop the index
    QVERIFY(connect.dropIndex(index1.name()));
}

void SqliteTests::cleanupTestCase()
{
    connect.close();
}

void SqliteTests::verifyCreateTable(){
    DQSqliteStatement statement;
    QString sql = statement.createTableIfNotExists<Model1>();

    QString answer = "CREATE TABLE IF NOT EXISTS model1  (\n"
    "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "key TEXT NOT NULL,\n"
    "value TEXT \n"
    ");";

    QCOMPARE(sql,answer);

    QString sql2 = statement.createTableIfNotExists<Model2>();
    sql2.replace("model2","model1");
    QCOMPARE(sql,sql2);

    sql = statement.createTableIfNotExists<AllType>();

    QStringList lines = sql.split("\n");

    int n = 8; // no. of field
    QVERIFY(lines.size()  == n +3);

}

void SqliteTests::foreignKey() {
    Model2 model;

    QVERIFY (model.metaInfo()->foreignKeyNameList().size() == 0 );

    DQForeignKey<Model2> key;


    model.key = "test key";
    model.value = "test value";

    key = model;

    QVERIFY(key->key() == "test key");

    DQSqliteStatement statement;
    QString sql = statement.createTableIfNotExists<Config>();


    QString answer = "CREATE TABLE IF NOT EXISTS config  (\n"
    "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "key TEXT ,\n"
    "value TEXT ,\n"
    "uid INTEGER NOT NULL,\n"
    "FOREIGN KEY(uid) REFERENCES user(id)\n"
    ");" ;

    QCOMPARE(sql,answer);

    Config config;
    QVERIFY (config.metaInfo()->foreignKeyNameList().size() == 1 );

}


void SqliteTests::insertInto(){
    DQSqliteStatement statement;
    DQModelMetaInfo *info = dqMetaInfo<Model2>();
    QString sql = statement.insertInto(info,info->fieldNameList());

    QVERIFY( sql == "INSERT INTO model2 (id,key,value) values (:id,:key,:value);" );

    sql = statement.replaceInto(info,info->fieldNameList());
    QVERIFY( sql == "REPLACE INTO model2 (id,key,value) values (:id,:key,:value);" );

    DQConnection connection = DQConnection::defaultConnection();
    QVERIFY (connection.isOpen() );


}

void SqliteTests::dqModelSave(){
    Model1 model1;

    QVERIFY(model1.id->isNull());

    model1.key = "save1";
    model1.value = "value1";

    QVERIFY ( model1.save() );
    QVERIFY (!model1.id->isNull());

    QVariant id = model1.id();

    QVERIFY ( model1.save() );
    QVERIFY ( model1.id() == id);

    QVERIFY ( model1.save(true) ); // Force insert
    QVERIFY ( model1.id() != id); // ID should be changed.
}

void SqliteTests::deleteAll() {
    DQQuery<Model1> query;

    QVERIFY(query.remove());

    int count = query.count();
    QVERIFY(count == 0);

    Model1 model1;
    model1.key = "temp";
    model1.value = "temp value";
    model1.save();

    query = DQQuery<Model1>();
    count = query.count();
    QVERIFY(count == 1);

    count = query.count(); // call it twice
    QVERIFY(count == 1);

    QVERIFY(!model1.id->isNull());
    QVERIFY(model1.remove());
    QVERIFY(model1.id->isNull());
    QVERIFY(query.count() == 0);
    QVERIFY(model1.save());
    QVERIFY(query.count() == 1);

    int id = model1.id().toInt();
    Model1 model1b;
    QVERIFY(model1b.load( DQWhere("id = ",id) ));
//    QVERIFY(model1.id() == model1b.id() );
    QVERIFY(model1.id == model1b.id );

    query = DQQuery<Model1>().filter(DQWhere("key = " ,"temp")).limit(1);
    QEXPECT_FAIL("","Normally sqlite do not support limit in delete from , unless it is build with special flag",Continue);
    QVERIFY(query.remove());

    qDebug() << "Failed sql : " << query.lastQuery().lastQuery();

    query = DQQuery<Model1>().filter(DQWhere("key" , "=" , "temp"));
    QVERIFY(query.remove());

    qDebug() << query.lastQuery().lastQuery();

    query = DQQuery<Model1>();

    count = query.count();
    QVERIFY(count == 0);


}

void SqliteTests::prepareInitRecords() {
    Model1 model1;
    model1.key = "config1";
    model1.value ="value1";
    QVERIFY ( model1.save(true) );

    model1.key = "config2";
    model1.value ="value2";
    QVERIFY ( model1.save(true) );

    model1.key = "config3";
    model1.value ="value3";
    QVERIFY ( model1.save(true) );

    Model2 model2;
    model2.key = "config1";
    model2.value = "value1";
    QVERIFY(model2.save());

    model2.key = "config2";
    model2.value = "value2";
    QVERIFY(model2.save(true));

    User user;
    user.name = "Ben Lau";
    user.userId = "benlau";

    QVERIFY(!user.save()); // passwd is missed

    user.passwd = "123456";
    QVERIFY (!user.save()); // passwd is too short
    user.passwd = "12345678";
    QVERIFY (user.save());

    Config config;
    config.key = "config1";
    config.value = "value1";
    QVERIFY(!config.save()); // config.uid can not be null

    QVERIFY(config.id->isNull());

    config.uid = user;

    QVERIFY(config.save());

    ExamResult result;
    result.uid = user;
    result.subject = "English";
    result.mark = 50;
    QVERIFY(result.save());

    result.subject = "Maths";
    result.mark = 80;
    QVERIFY(result.save(true));
    DQQuery<ExamResult> examResultQuery;
    QVariant total = examResultQuery.filter(DQWhere("uid = " , user.id )).call("sum",QStringList("mark"));
    QVERIFY(total == 130);
    QVariant avg = examResultQuery.filter(DQWhere("uid = " , user.id() )).call("avg",QStringList("mark"));
    QVERIFY(avg == 65);

}

void SqliteTests::select()
{
    Model1 model1a,model1b;

    DQQuery<Model1> query1 = DQQuery<Model1>().filter(DQWhere("key","=","config1")).limit(1) ;

    DQSqliteStatement statement;
    QString sql = statement.select(query1);

    qDebug() << sql;

    QVERIFY (query1.exec() );
    QVERIFY (query1.next() );

    QVERIFY( query1.recordTo(model1a)) ;

    QVERIFY( !model1a.id->isNull());
    QVERIFY( model1a.key() == "config1");
    QVERIFY( model1a.value() == "value1");

    model1b = query1.record();

    QVERIFY( !model1b.id->isNull());
    QVERIFY( model1b.key() == "config1");
    QVERIFY( model1b.value() == "value1");

    QVERIFY( !query1.next());

}

void SqliteTests::queryAll(){
    DQQuery<Model2> query;

    DQList<Model2> record = query.all();

    QVERIFY(record.size() == 7); // 5 initial record + 2 newly inserted record
    QVERIFY(record.at(0)->key() == "initial0");
    QVERIFY(record.at(0)->value() == "value0");
    QVERIFY(record.at(1)->key() == "initial1");
    QVERIFY(record.at(1)->value() == "value1");

    // Alernative way

    record = Model2::objects().all();
    QVERIFY(record.size() == 7);

}

void SqliteTests::querySelect() {
    DQQuery<Model2> query;

    DQList<Model2> record = query.select("key").all();

    QVERIFY(record.size() == 7); // 5 initial record + 2 newly inserted record
    QVERIFY(record.at(0)->key() == "initial0");
    QVERIFY(record.at(0)->value().isNull()); // It is not loaded
    QVERIFY(record.at(1)->key() == "initial1");
    QVERIFY(record.at(1)->value().isNull()); // It is not loaded

    // Alernative way

    record = Model2::objects().all();
    QVERIFY(record.size() == 7);

}

void SqliteTests::querySelectWhere(){
    DQQuery<HealthCheck> query;

    QVERIFY(query.remove());

    DQList<HealthCheck> list;
    DQListWriter writer(&list);

    writer << "Tester 1 - Alvin" << 180 << 150 << writer.next()
           << "Tester 2 - Ben" << 170 << 120 << writer.next()
           << "Tester 3 - Candy" << 150 << 180 << writer.next()
           << "Tester 4 - David" << 130 << 130 << writer.next();

    list.save();

    query =query.filter(DQWhere("height") == (DQWhere("weight")));
    list = query.all();

    QString sql = query.lastQuery().lastQuery();
    qDebug() << sql;
    QVERIFY(sql == "SELECT ALL * FROM healthcheck WHERE height = weight ;");

    QVERIFY(list.size() == 1); // Tester 4;

    query.reset();
    QVERIFY(query.filter(DQWhere("height") != 130 ).all().size() == 3);
    QVERIFY(query.filter(DQWhere("height").isNot(130) ).all().size() == 3);
    QVERIFY(query.filter(DQWhere("height").is(130) ).all().size() == 1);

    query.reset();
    query =query.filter(DQWhere("height").between(150,170));
    list = query.all();
//    qDebug() << query.lastQuery().lastQuery();
    QVERIFY(list.size() == 2);

    query.reset();
    QList<QVariant> range;
    range << 120 << 130 << 150 << 300;

    query =query.filter(DQWhere("weight").in(range));
    list = query.all();
    qDebug() << query.lastQuery().lastQuery();
    QVERIFY(list.size() == 3);

    query.reset();
    QVERIFY(query.filter(DQWhere("weight").notIn(range)).all().size() == 1);

    QVERIFY(query.filter(DQWhere("name").like("Tester%")).all().size() == 4);
    QVERIFY(query.filter(DQWhere("name").like("Tester")).all().size() == 0);

    QCOMPARE(query.filter(DQWhere("name").glob("Tester*")).all().size() , 4);

    QVERIFY(query.remove());
}

void SqliteTests::foreignKeyLoad() {
    User user;
    user.name = "foreignKeyLoad";
    user.userId = "foreignKeyLoad";
    user.passwd = "12345678";

    QVERIFY(user.save());
    Config config1;
    config1.key = "autoLogin";
    config1.value = "true";
    QVERIFY(!config1.save()); // config.uid can not be null
    QVERIFY(config1.id->isNull());

    QVERIFY(!config1.uid.isLoaded());

    config1.uid = user; // Test can DQForeignKey read from DQModel.
    QVERIFY(config1.save());

    QVERIFY(config1.uid.isLoaded());

    DQQuery<Config> query = DQQuery<Config>().filter(DQWhere("key = ", "autoLogin")).limit(1);

    QVERIFY(query.exec());
    QVERIFY(query.next());

    Config config2 = query.record();
    QVERIFY(config2.key() == "autoLogin");

    QVERIFY(!config2.uid.isLoaded());
    QVERIFY(config2.uid->name() == "foreignKeyLoad");
    QVERIFY(config2.uid.isLoaded());
}

void SqliteTests::model4() {
    Model4 item1,item2;

    item1.key = "test";
    item1.value = "test";
    QVERIFY (item1.save());

//    qDebug() << item1.lastQuery().lastQuery();
    qDebug() << &item1;

    QVERIFY (item2.load(DQWhere("key = ","test")));

    QVERIFY(item2.help == "...");

}

void SqliteTests::datetime() {
    User user1;

    user1.userId = "tester";
    user1.name = "tester";
    user1.passwd = "12345678";

    QDateTime datetime = QDateTime::currentDateTime().toUTC();

    user1.lastLoginTime = datetime;

    QVERIFY(user1.clean());
    QVERIFY(user1.save());

    User user2;
    QVERIFY(user2.load(DQWhere("userId=","tester")));

#if (QT_VERSION > QT_VERSION_CHECK(5, 0, 2))
    // The timezone checking do not exists in Qt 5.0.2
    QEXPECT_FAIL("","Sqlite driver do not save the time zone of QDateTime type",Continue);
#endif

    QVERIFY(user2.lastLoginTime == datetime);

    QString format("yyyy-MM-ddThh:mm:ss");
    QVERIFY(user2.lastLoginTime.get().toDateTime().toString(format) == datetime.toString(format));

    QVERIFY(!user2.creationTime->isNull());
}

void SqliteTests::checkTypeSaveAndLoad(){

    QStringList sl;
    sl << "a" << "b" << "c";

    AllType type1,type2;
    type1.d = "1.0";
    QByteArray data(100,'0');
    type1.data = data;
    type1.b = true;
    type1.sl = sl;

    QVERIFY(type1.save());

    QVERIFY(type2.load(DQWhere("id=",type1.id) ) );

    QVERIFY(type2.data == data);
    QVERIFY(type2.d == "1.0");
    QVERIFY(type2.b == true);
    qDebug() << type2.sl;
    QVERIFY(type2.sl == sl);

    // try again

    type1.b = false;

    QVERIFY(type1.save());

    QVERIFY(type2.load(DQWhere("id=",type1.id) ) );

    QVERIFY(type2.b == false);

}

void SqliteTests::queryOrderBy(){
    DQQuery<HealthCheck> query;

    QVERIFY(query.remove());

    DQList<HealthCheck> records;

    DQListWriter writer(&records);

    writer << "tester 1" << 160 << 120 << writer.next()
           << "tester 2" << 120 << 170 << writer.next()
           << "tester 3" << 140 << 110 << writer.next();

    writer.close();
    QVERIFY(records.save());

    DQList<HealthCheck> result = query.all();
    QVERIFY(result.size() == 3);

    result = query.orderBy("height asc").all();

    QVERIFY(result.size() == 3);

    QVERIFY(result.at(0)->name == "tester 2");
    QVERIFY(result.at(2)->name == "tester 1");

    result = query.orderBy("height desc").all();
    QVERIFY(result.at(0)->name == "tester 1");
    QVERIFY(result.at(2)->name == "tester 2");

    result = query.orderBy("weight desc").all();
    QVERIFY(result.at(0)->name == "tester 2");
    QVERIFY(result.at(2)->name == "tester 3");

    result = query.orderBy("weight asc").all();
    QVERIFY(result.at(0)->name == "tester 3");
    QVERIFY(result.at(2)->name == "tester 2");

}

