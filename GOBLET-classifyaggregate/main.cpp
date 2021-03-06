#include <QObject>
#include "mydbconn.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QTime>
#include <tclap/CmdLine.h>
#include <QPointF>
#include <QCoreApplication>

struct classInfo
{
  long classNumber;
  double from;
  double to;
};
typedef classInfo TclassInfo;

QList<classInfo> classes;

void separateClasses(QString classData)
{
    if (classData.length()==0)
        return;

    if (classData.indexOf(" ") < 0)
        return;
    int pos;
    QString temp;
    temp = classData;
    pos = temp.indexOf(":");
    TclassInfo classDef;
    classDef.classNumber = temp.left(pos).toInt();

    //mid(pos+1,classData.length()-pos+1);
    temp = temp.mid(pos+1,temp.length()-pos+1);
    pos = temp.indexOf(" ");
    classDef.from = temp.left(pos).toDouble();
    classDef.to = temp.mid(pos+1,temp.length()-pos+1).toDouble();
    classes.append(classDef);
}

void getClasses(QString classDef)
{
   //ClassNumber:valueFrom ValueTo,ClassNumber:valueFrom ValueTo,...
    int totalClasses = classDef.count(":");
    if (totalClasses == 0)
    {
        gbtLog("Error in class definition");
        return;
    }
    int totcomas;
    totcomas = classDef.count(",");
    if (!(totalClasses-1 == totcomas))
    {
        gbtLog("Error in class definition");
        return;
    }
    QString classData;
    QString temp;
    classData = classDef;
    int pos;
    pos = 0;
    while ((pos <= classData.length()-1))
    {
        if (classData[pos] != ',')
        {
            temp = temp + classData[pos];
            pos++;
        }
        else
        {
            separateClasses(temp);

            temp = classData.mid(pos+1,classData.length()-pos+1);
            classData = temp;

            temp = "";
            pos = 0;
        }
    }
    separateClasses(temp);
}

QString getWhereClauseFromExtent(QString extent)
{
    //(1.3333,32.1212321) (-4.12121,41.212121)
    if (!extent.count(" ") == 1)
    {
        gbtLog(QObject::tr("Extent is invalid"));
        return QString();
    }
    if (!extent.count(",") == 2)
    {
        gbtLog(QObject::tr("Extent is invalid"));
        return QString();
    }
    if (!extent.count("(") == 2)
    {
        gbtLog(QObject::tr("Extent is invalid"));
        return QString();
    }
    if (!extent.count(")") == 2)
    {
        gbtLog(QObject::tr("Extent is invalid"));
        return QString();
    }
    int pos;
    pos = extent.indexOf(" ");
    QString from;
    QString to;
    from = extent.left(pos);
    to = extent.mid(pos+1,extent.length()-pos+1);
    from.replace("(","");
    from.replace(")","");
    to.replace("(","");
    to.replace(")","");

    //Get UpperLeft
    QPointF dupperLeft;
    pos = from.indexOf(",");
    dupperLeft.setX(from.left(pos).toDouble());
    dupperLeft.setY(from.mid(pos+1,from.length()-pos+1).toDouble());


    //Get lower right
    QPointF dlowerRight;
    pos = to.indexOf(",");
    dlowerRight.setX(to.left(pos).toDouble());
    dlowerRight.setY(to.mid(pos+1,to.length()-pos+1).toDouble());

    QString res;
    res = "contains(PolygonFromText('POLYGON((";
    res = res + QString::number(dupperLeft.x(),'f',15) + " ";
    res = res + QString::number(dupperLeft.y(),'f',15) + ", ";

    res = res + QString::number(dlowerRight.x(),'f',15) + " ";
    res = res + QString::number(dupperLeft.y(),'f',15) + ", ";

    res = res + QString::number(dlowerRight.x(),'f',15) + " ";
    res = res + QString::number(dlowerRight.y(),'f',15) + ", ";

    res = res + QString::number(dupperLeft.x(),'f',15) + " ";
    res = res + QString::number(dlowerRight.y(),'f',15) + ", ";

    res = res + QString::number(dupperLeft.x(),'f',15) + " ";
    res = res + QString::number(dupperLeft.y(),'f',15);

    res = res + "))'),TB.ogc_geom)";


    return res;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    //Command line arguments
    TCLAP::CmdLine cmd("GOBLET (c) 2012, International Livestock Research Institute (ILRI) \n Developed by Carlos Quiros (c.f.quiros@cgiar.org)", ' ', "1.0 (Beta 1)");
    //Required arguments
    TCLAP::ValueArg<std::string> databaseArg("d","database","Database name",true,"","string");
    TCLAP::ValueArg<std::string> datasetArg("t","dataset","Dataset name",true,"","string");
    TCLAP::ValueArg<std::string> shapeArg("s","shapedataset","The aggregated shape dataset",true,"","string");
    TCLAP::ValueArg<std::string> classArg("c","classdefinition","Class definition: 'ClassNumber:valueFrom ValueTo,ClassNumber:valueFrom ValueTo,...'",true,"","string");

    //Non required arguments
    TCLAP::ValueArg<std::string> pathArg("a","path","Path to database. Default .",false,".","string");
    TCLAP::ValueArg<std::string> hostArg("H","host","Connect to host. Default localhost",false,"localhost","string");
    TCLAP::ValueArg<std::string> portArg("P","port","Port number to use. Default 3306",false,"3306","string");
    TCLAP::ValueArg<std::string> userArg("u","user","User. Default empty",false,"","string");
    TCLAP::ValueArg<std::string> passArg("p","password","Passwork. Default no password",false,"","string");
    TCLAP::ValueArg<std::string> extentArg("e","extent","Extent: '(upperLeft degrees lat,log) (lowerRight degrees lat,log)'",false,"","string");


    //Switches
    TCLAP::SwitchArg remoteSwitch("r","remote","Connect to remote host", cmd, false);
    cmd.add(databaseArg);
    cmd.add(datasetArg);
    cmd.add(shapeArg);
    cmd.add(classArg);

    cmd.add(pathArg);
    cmd.add(hostArg);
    cmd.add(portArg);
    cmd.add(userArg);
    cmd.add(passArg);

    cmd.add(extentArg);



    //Parsing the command lines
    cmd.parse( argc, argv );

    //Getting the variables from the command
    bool remote = remoteSwitch.getValue();
    QString path = QString::fromUtf8(pathArg.getValue().c_str());
    QString dbName = QString::fromUtf8(databaseArg.getValue().c_str());
    QString host = QString::fromUtf8(hostArg.getValue().c_str());
    QString port = QString::fromUtf8(portArg.getValue().c_str());
    QString userName = QString::fromUtf8(userArg.getValue().c_str());
    QString password = QString::fromUtf8(passArg.getValue().c_str());
    QString tableName = QString::fromUtf8(datasetArg.getValue().c_str());
    QString classDef = QString::fromUtf8(classArg.getValue().c_str());
    QString extent = QString::fromUtf8(extentArg.getValue().c_str());
    QString shapeDataSet = QString::fromUtf8(shapeArg.getValue().c_str());

    myDBConn con;
    QSqlDatabase mydb;
    if (!remote)
    {
        QDir dir;
        dir.setPath(path);
        if (con.connectToDB(dir.absolutePath()) == 1)
        {
            if (!dir.cd(dbName))
            {
                gbtLog(QObject::tr("The database does not exists"));
                con.closeConnection();
                return 1;
            }
            mydb = QSqlDatabase::addDatabase(con.getDriver(),"connection1");
        }
    }
    else
    {
        mydb = QSqlDatabase::addDatabase("QMYSQL","connection1");
        mydb.setHostName(host);
        mydb.setPort(port.toInt());
        if (!userName.isEmpty())
           mydb.setUserName(userName);
        if (!password.isEmpty())
           mydb.setPassword(password);
    }

    mydb.setDatabaseName(dbName);

    if (!mydb.open())
    {
        gbtLog(QObject::tr("Cannot open database"));
        con.closeConnection();
        return 1;
    }
    else
    {

        getClasses(classDef);
        if (classes.count() ==0 )
        {
            gbtLog(QObject::tr("There are no classes!"));
            mydb.close();
            con.closeConnection();
            return 1;
        }

        QTime procTime;
        procTime.start();

        QString sql;
        QSqlQuery qry(mydb);

        sql = "SELECT count(*) from datasetinfo WHERE dataset_id = '" + tableName + "' and dataset_type = 1";
        if (qry.exec(sql))
        {
            qry.first();
            if (qry.value(0).toInt() == 0)
            {
                gbtLog(QObject::tr("Dataset ") + tableName + QObject::tr(" does not exists"));
                mydb.close();
                con.closeConnection();
                return 1;
            }
        }
        else
        {
            gbtLog(QObject::tr("Cannot classify dataset."));
            gbtLog(qry.lastError().databaseText());
            mydb.close();
            con.closeConnection();
            return 1;
        }

        sql = "SELECT count(*) from datasetinfo WHERE dataset_id = '" + shapeDataSet + "' and dataset_type = 2";
        if (qry.exec(sql))
        {
            qry.first();
            if (qry.value(0).toInt() == 0)
            {
                gbtLog(QObject::tr("Shapefile ") + shapeDataSet + QObject::tr(" does not exists"));
                mydb.close();
                con.closeConnection();
                return 1;
            }
        }
        else
        {
            gbtLog(QObject::tr("Cannot classify dataset."));
            gbtLog(qry.lastError().databaseText());
            mydb.close();
            con.closeConnection();
            return 1;
        }

        sql = "UPDATE aggrtable TA, " + shapeDataSet  + " TB SET TA.classCode = CASE";

        for (int t=0; t <= classes.count()-1;t++)
        {
            sql = sql + " WHEN (TA.shapevalue >= " + QString::number(classes[t].from,'f',5);
            sql = sql + " AND TA.shapevalue <= " + QString::number(classes[t].to,'f',5);
            sql = sql + ") THEN " + QString::number(classes[t].classNumber);
        }
        sql = sql + " ELSE null END WHERE TA.griddataset = '" + tableName + "'";
        sql = sql + " AND TA.shapedataset = '" + shapeDataSet + "' AND TA.shapeid = TB.shapeid";

        QString WhereClause;
        if (!extent.isEmpty())
        {
            WhereClause = getWhereClauseFromExtent(extent);
            if (!WhereClause.isEmpty())
                sql = sql + " AND " + WhereClause;
        }


        gbtLog(QObject::tr("Classifying...Please wait"));

        if (!qry.exec(sql))
        {
            gbtLog(QObject::tr("Cannot classify dataset."));
            gbtLog(qry.lastError().databaseText());
            mydb.close();
            con.closeConnection();
            return 1;
        }

        int Hours;
        int Minutes;
        int Seconds;
        int Milliseconds;

        Milliseconds = procTime.elapsed();

        Hours = Milliseconds / (1000*60*60);
        Minutes = (Milliseconds % (1000*60*60)) / (1000*60);
        Seconds = ((Milliseconds % (1000*60*60)) % (1000*60)) / 1000;

        gbtLog("Finished in " + QString::number(Hours) + " Hours," + QString::number(Minutes) + " Minutes and " + QString::number(Seconds) + " Seconds.");

        mydb.close();
        con.closeConnection();

        return 0;
    }

    return 0;

}
