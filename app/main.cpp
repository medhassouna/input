/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QtDebug>
#include <QQmlError>
#include <QDesktopWidget>
#include <QWindow>
#include <QtGlobal>
#include <QQmlContext>
#include <QQuickWindow>
#include <QLocale>
#ifdef INPUT_TEST
#include "test/inputtests.h"
#endif
#include <qqml.h>
#include <qgsmessagelog.h>
#include "qgsconfig.h"
#include "qgsproviderregistry.h"
#include "qgsmaplayerproxymodel.h"

#include "androidutils.h"
#include "ios/iosutils.h"
#include "inpututils.h"
#include "coreutils.h"
#include "position/positiondirection.h"
#include "mapthemesmodel.h"
#include "merginapi.h"
#include "merginapistatus.h"
#include "merginsubscriptioninfo.h"
#include "merginsubscriptionstatus.h"
#include "merginsubscriptiontype.h"
#include "merginprojectstatusmodel.h"
#include "layersproxymodel.h"
#include "layersmodel.h"
#include "activelayer.h"
#include "purchasing.h"
#include "merginuserauth.h"
#include "merginuserinfo.h"
#include "variablesmanager.h"
#include "inputhelp.h"
#include "inputprojutils.h"
#include "fieldsmodel.h"
#include "projectwizard.h"
#include "codefilter.h"
#include "inputexpressionfunctions.h"
#include "compass.h"
#include "attributepreviewcontroller.h"
#include "qgsfeature.h"
#include "qgslogger.h"
#include "qgsmaplayer.h"
#include "qgsmessagelog.h"
#include "qgspointxy.h"
#include "qgsproject.h"
#include "qgsrelationmanager.h"
#include "qgscoordinatetransformcontext.h"
#include "qgsvectorlayer.h"
#include "qgsunittypes.h"

#include "rememberattributescontroller.h"
#include "attributecontroller.h"
#include "attributedata.h"
#include "attributeformmodel.h"
#include "attributeformproxymodel.h"
#include "attributetabmodel.h"
#include "attributetabproxymodel.h"
#include "qgsquickcoordinatetransformer.h"
#include "identifykit.h"
#include "featurelayerpair.h"
#include "qgsquickmapcanvasmap.h"
#include "qgsquickmapsettings.h"
#include "qgsquickmaptransform.h"
#include "position/positionkit.h"
#include "scalebarkit.h"
#include "featuresmodel.h"
#include "relationfeaturesmodel.h"
#include "relationreferencefeaturesmodel.h"
#include "fieldvalidator.h"
#include "valuerelationfeaturesmodel.h"
#include "snaputils.h"
#include "guidelinecontroller.h"

#include "projectsmodel.h"
#include "projectsproxymodel.h"
#include "project.h"
#include "qgsproject.h"
#include "bluetoothdiscoverymodel.h"
#include "position/mapposition.h"
#include "position/positionprovidersmodel.h"
#include "position/abstractpositionprovider.h"
#include "synchronizationmanager.h"
#include "synchronizationerror.h"

#include "maptools/abstractmaptool.h"
#include "maptools/recordingmaptool.h"
#include "maptools/splittingmaptool.h"

#ifndef NDEBUG
// #include <QQmlDebuggingEnabler>
#endif

#ifdef MOBILE_OS
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#endif

#ifdef ANDROID
#include <QtAndroidExtras>
#endif

#ifdef DESKTOP_OS
#include <QCommandLineParser>
#include <qgis.h>
#endif

#include "qgsapplication.h"
#include "activeproject.h"
#include "appsettings.h"

static QString getDataDir()
{
#ifdef QGIS_QUICK_DATA_PATH
  QString dataPathRaw( STR( QGIS_QUICK_DATA_PATH ) );

#ifdef ANDROID
  dataPathRaw = AndroidUtils::externalStorageAppFolder();
#endif

#ifdef Q_OS_IOS
  QString docsLocation = QStandardPaths::standardLocations( QStandardPaths::DocumentsLocation ).value( 0 );

  QDir myDir( docsLocation );
  if ( !myDir.exists() )
  {
    myDir.mkpath( docsLocation );
  }
  dataPathRaw = docsLocation + "/" + dataPathRaw;
#endif

#ifdef Q_OS_WIN32
  QString appLocation = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );

  QDir myDir( appLocation );
  if ( !myDir.exists() )
  {
    myDir.mkpath( appLocation );
  }
  dataPathRaw = appLocation + "/" + dataPathRaw;
#endif

  qputenv( "QGIS_QUICK_DATA_PATH", dataPathRaw.toUtf8().constData() );
#else
  qDebug( "== Must set QGIS_QUICK_DATA_PATH in order to get QGIS Quick running! ==" );
#endif
  QString dataDir = QString::fromLocal8Bit( qgetenv( "QGIS_QUICK_DATA_PATH" ) ) ;
  qDebug() << "QGIS_QUICK_DATA_PATH: " << dataDir;
  return dataDir;
}

static void setEnvironmentQgisPrefixPath()
{
#ifdef DESKTOP_OS
#ifdef QGIS_PREFIX_PATH
  qputenv( "QGIS_PREFIX_PATH", STR( QGIS_PREFIX_PATH ) );
#endif
  if ( QString::fromLocal8Bit( qgetenv( "QGIS_PREFIX_PATH" ) ).isEmpty() )
  {
    // if not on Android, QGIS_PREFIX_PATH env variable should have been set already or defined as C++ define
    qDebug( "== Must set QGIS_PREFIX_PATH in order to get QGIS Quick module running! ==" );
  }
#endif

#if defined (ANDROID) || defined (Q_OS_IOS)
  QDir myDir( QDir::homePath() );
  myDir.cdUp();
  QString prefixPath = myDir.absolutePath();  // something like: /data/data/org.qgis.quick
  qputenv( "QGIS_PREFIX_PATH", prefixPath.toUtf8().constData() );
#elif defined (Q_OS_WIN32)
  QString prefixPath = QCoreApplication::applicationDirPath();
  qputenv( "QGIS_PREFIX_PATH", prefixPath.toUtf8().constData() );
#endif

  qDebug() << "QGIS_PREFIX_PATH: " << QString::fromLocal8Bit( qgetenv( "QGIS_PREFIX_PATH" ) );
}


static void copy_demo_projects( const QString &demoDir, const QString &projectDir )
{
  if ( !demoDir.isEmpty() )
    InputUtils::cpDir( demoDir, projectDir );

  QFile demoFile( projectDir + "/Start here!/qgis-project.qgz" );
  if ( demoFile.exists() )
    qDebug() << "DEMO projects initialized";
  else
    CoreUtils::log( QStringLiteral( "DEMO" ), QStringLiteral( "The Input has failed to initialize demo projects" ) );
}

static void init_qgis( const QString &pkgPath )
{
  QgsApplication::init();

#ifdef MOBILE_OS
  // QGIS plugins on Android are in the same path as other libraries
  QgsApplication::setPluginPath( QApplication::applicationDirPath() );
  QgsApplication::setPkgDataPath( pkgPath );
#else
  Q_UNUSED( pkgPath )
#endif

  QgsApplication::initQgis();

  // make sure the DB exists - otherwise custom projections will be failing
  if ( !QgsApplication::createDatabase() )
    qDebug( "Can't create qgis user DB!!!" );

  qDebug( "qgis providers:\n%s", QgsProviderRegistry::instance()->pluginList().toLatin1().data() );
}

void initDeclarative()
{
  qmlRegisterUncreatableType<MerginUserAuth>( "lc", 1, 0, "MerginUserAuth", "" );
  qmlRegisterUncreatableType<MerginUserInfo>( "lc", 1, 0, "MerginUserInfo", "" );
  qmlRegisterUncreatableType<MerginSubscriptionInfo>( "lc", 1, 0, "MerginSubscriptionInfo", "" );
  qmlRegisterUncreatableType<PurchasingPlan>( "lc", 1, 0, "MerginPlan", "" );
  qmlRegisterUncreatableType<MapThemesModel>( "lc", 1, 0, "MapThemesModel", "" );
  qmlRegisterUncreatableType<ActiveProject>( "lc", 1, 0, "ActiveProject", "" );
  qmlRegisterUncreatableType<SynchronizationManager>( "lc", 1, 0, "SynchronizationManager", "" );
  qmlRegisterUncreatableType<SynchronizationError>( "lc", 1, 0, "SyncError", "SyncError Enum" );
  qmlRegisterUncreatableType<AppSettings>( "lc", 1, 0, "AppSettings", "" );
  qmlRegisterUncreatableType<MerginApiStatus>( "lc", 1, 0, "MerginApiStatus", "MerginApiStatus Enum" );
  qmlRegisterUncreatableType<MerginSubscriptionStatus>( "lc", 1, 0, "MerginSubscriptionStatus", "MerginSubscriptionStatus Enum" );
  qmlRegisterUncreatableType<MerginSubscriptionType>( "lc", 1, 0, "MerginSubscriptionType", "MerginSubscriptionType Enum" );
  qmlRegisterUncreatableType<MerginProjectStatusModel>( "lc", 1, 0, "MerginProjectStatusModel", "Enum" );
  qmlRegisterUncreatableType<LayersModel>( "lc", 1, 0, "LayersModel", "" );
  qmlRegisterUncreatableType<LayersProxyModel>( "lc", 1, 0, "LayersProxyModel", "" );
  qmlRegisterUncreatableType<ActiveLayer>( "lc", 1, 0, "ActiveLayer", "" );
  qmlRegisterType<PositionDirection>( "lc", 1, 0, "PositionDirection" );
  qmlRegisterType<Compass>( "lc", 1, 0, "Compass" );
  qmlRegisterType<FieldsModel>( "lc", 1, 0, "FieldsModel" );
  qmlRegisterType<CodeFilter>( "lc", 1, 0, "CodeFilter" );
  qmlRegisterType<ProjectsModel>( "lc", 1, 0, "ProjectsModel" );
  qmlRegisterType<ProjectsProxyModel>( "lc", 1, 0, "ProjectsProxyModel" );
  qmlRegisterType<AttributePreviewController>( "lc", 1, 0, "AttributePreviewController" );
  qmlRegisterUncreatableType<AttributePreviewModel>( "lc", 1, 0, "AttributePreviewModel", "" );
  qmlRegisterUncreatableMetaObject( ProjectStatus::staticMetaObject, "lc", 1, 0, "ProjectStatus", "ProjectStatus Enum" );
  qRegisterMetaType< FeatureLayerPair >( "FeatureLayerPair" );
  qRegisterMetaType< FeatureLayerPair * >( "FeatureLayerPair*" );
  qRegisterMetaType< AttributeController * >( "AttributeController*" );

  qRegisterMetaType< QList<QgsMapLayer *> >( "QList<QgsMapLayer*>" );
  qRegisterMetaType< QgsAttributes > ( "QgsAttributes" );
  qRegisterMetaType< QgsCoordinateReferenceSystem >( "QgsCoordinateReferenceSystem" );
  qRegisterMetaType< QgsCoordinateTransformContext >( "QgsCoordinateTransformContext" );
  qRegisterMetaType< QgsFeature > ( "QgsFeature" );
  qRegisterMetaType< QgsFeatureId > ( "QgsFeatureId" );
  qRegisterMetaType< QgsPoint >( "QgsPoint" );
  qRegisterMetaType< QgsPointXY >( "QgsPointXY" );
  qRegisterMetaType< QgsRelation >( "QgsRelation" );
  qRegisterMetaType< QgsPolymorphicRelation >( "QgsPolymorphicRelation" );
  qRegisterMetaType< QgsUnitTypes::SystemOfMeasurement >( "QgsUnitTypes::SystemOfMeasurement" );
  qRegisterMetaType< QgsUnitTypes::DistanceUnit >( "QgsUnitTypes::DistanceUnit" );
  qRegisterMetaType< QgsCoordinateFormatter::FormatFlags >( "QgsCoordinateFormatter::FormatFlags" );
  qRegisterMetaType< QgsCoordinateFormatter::Format >( "QgsCoordinateFormatter::Format" );
  qRegisterMetaType< QVariant::Type >( "QVariant::Type" );

  qmlRegisterUncreatableType< FormItem >( "lc", 1, 0, "FormItem", "Only enums from FormItem can be used" );
  qmlRegisterUncreatableType< AttributeFormModel >( "lc", 1, 0, "AttributeFormModel", "Created by AttributeController" );
  qmlRegisterUncreatableType< AttributeFormProxyModel >( "lc", 1, 0, "AttributeFormProxyModel", "Created by AttributeController" );
  qmlRegisterUncreatableType< AttributeTabModel >( "lc", 1, 0, "AttributeTabModel", "Created by AttributeController" );
  qmlRegisterUncreatableType< AttributeTabProxyModel >( "lc", 1, 0, "AttributeTabProxyModel", "Created by AttributeController" );
  qmlRegisterUncreatableType< FieldValidator >( "lc", 1, 0, "FieldValidator", "Only enums from FieldValidator can be used" );
  qmlRegisterType< AttributeController >( "lc", 1, 0, "AttributeController" );
  qmlRegisterType< RememberAttributesController >( "lc", 1, 0, "RememberAttributesController" );
  qmlRegisterType< IdentifyKit >( "lc", 1, 0, "IdentifyKit" );
  qmlRegisterType< PositionKit >( "lc", 1, 0, "PositionKit" );
  qmlRegisterType< MapPosition >( "lc", 1, 0, "MapPosition" );
  qmlRegisterType< ScaleBarKit >( "lc", 1, 0, "ScaleBarKit" );
  qmlRegisterType< SnapUtils >( "lc", 1, 0, "SnapUtils" );
  qmlRegisterType< GuidelineController >( "lc", 1, 0, "GuidelineController" );
  qmlRegisterType< FeaturesModel >( "lc", 1, 0, "FeaturesModel" );
  qmlRegisterType< RelationFeaturesModel >( "lc", 1, 0, "RelationFeaturesModel" );
  qmlRegisterType< ValueRelationFeaturesModel >( "lc", 1, 0, "ValueRelationFeaturesModel" );
  qmlRegisterType< RelationReferenceFeaturesModel >( "lc", 1, 0, "RelationReferenceFeaturesModel" );
  qmlRegisterType< BluetoothDiscoveryModel >( "lc", 1, 0, "BluetoothDiscoveryModel" );
  qmlRegisterType< PositionProvidersModel >( "lc", 1, 0, "PositionProvidersModel" );

  qmlRegisterUncreatableType< QgsUnitTypes >( "QgsQuick", 0, 1, "QgsUnitTypes", "Only enums from QgsUnitTypes can be used" );
  qmlRegisterType< QgsVectorLayer >( "QgsQuick", 0, 1, "VectorLayer" );
  qmlRegisterType< QgsProject >( "QgsQuick", 0, 1, "Project" );
  qmlRegisterType< QgsQuickMapCanvasMap >( "QgsQuick", 0, 1, "MapCanvasMap" );
  qmlRegisterType< QgsQuickMapSettings >( "QgsQuick", 0, 1, "MapSettings" );
  qmlRegisterType< QgsQuickMapTransform >( "QgsQuick", 0, 1, "MapTransform" );
  qmlRegisterType< QgsQuickCoordinateTransformer >( "QgsQuick", 0, 1, "CoordinateTransformer" );

  qmlRegisterUncreatableType< AbstractPositionProvider >( "lc", 1, 0, "PositionProvider", "Must be instantiated via its construct method" );

  // map tools
  qmlRegisterUncreatableType< AbstractMapTool >( "lc", 1, 0, "AbstractMapTool", "Instantiate one of child map tools instead" );
  qmlRegisterType< RecordingMapTool >( "lc", 1, 0, "RecordingMapTool" );
  qmlRegisterType< SplittingMapTool >( "lc", 1, 0, "SplittingMapTool" );

  qmlRegisterType( QUrl( "qrc:/qgsquickmapcanvas.qml" ), "QgsQuick", 0, 1, "MapCanvas" );
}

void addQmlImportPath( QQmlEngine &engine )
{
  // This adds a runtime qml directory containing QgsQuick plugin
  // when Input is installed (e.g. Android/Win32)
  engine.addImportPath( QgsApplication::qmlImportPath() );
  qDebug() << "adding QML import Path: " << QgsApplication::qmlImportPath();

#ifdef QML_BUILD_IMPORT_DIR
  // Adds a runtime qml directory containing QgsQuick plugin
  // if we are using the developer mode (not installed Input)
  // e.g. Linux/MacOS
  QString qmlBuildImportPath( STR( QML_BUILD_IMPORT_DIR ) );
  engine.addImportPath( qmlBuildImportPath );
  qDebug() << "adding QML import Path: " << qmlBuildImportPath;
#endif

#ifdef Q_OS_IOS
  // REQUIRED FOR IOS - to load QgsQuick/*.qml files defined in qmldir
  engine.addImportPath( "qrc:///" );
  qDebug() << "adding QML import Path: " << "qrc:///";
#endif
}

int main( int argc, char *argv[] )
{
  // This flag enables auto scaling for HighDPI screens.
  // This basically means that each specified pixel in QML is now considered dp
  // See __dp comment for more information.
  QGuiApplication::setAttribute( Qt::AA_EnableHighDpiScaling );

  QgsApplication app( argc, argv, true );

  const QString version = CoreUtils::appVersion();

  // Set up the QSettings environment must be done after qapp is created
  QCoreApplication::setOrganizationName( "Lutra Consulting" );
  QCoreApplication::setOrganizationDomain( "lutraconsulting.co.uk" );
  QCoreApplication::setApplicationName( "Input" ); // used by QSettings
  QCoreApplication::setApplicationVersion( version );

#ifdef INPUT_TEST
  InputTests tests;
  tests.parseArgs( argc, argv );
#endif
  qDebug() << "Built with QGIS version " << VERSION_INT;

  // Set/Get enviroment
  QString dataDir = getDataDir();
  QString projectDir = dataDir + "/projects";

#ifdef INPUT_TEST
  if ( tests.testingRequested() )
  {
    projectDir = tests.initTestingDir();
  }
#endif

  QDir projectsDirectory( projectDir );
  if ( !projectsDirectory.exists() )
  {
    projectsDirectory.mkpath( projectDir );
  }

  CoreUtils::setLogFilename( projectDir + "/.logs" );
  setEnvironmentQgisPrefixPath();

  // Initialize translations
  QLocale locale;

  /*
   * We need to fix locale country code (capital letters after underscore)
   * in system locale, because QT tries to do exact match, for example:
   * system locale "en_GB" -> our provided locale "en" would not match and
   * QT would try to find translation for other system language.
   *
   * If it fails it goes through the same process again, but without country code.
   * We want to match "en_GB" with "en" immediately, thus we remove the country code.
   *
   * This fix can be removed from QT 5.15
   * See: https://github.com/MerginMaps/input/issues/1417
   * See: QTBUG-86179
   */
  locale = InputUtils::fixLocaleCountry( locale );

  QTranslator inputTranslator;
  if ( inputTranslator.load( locale, "input", "_", ":/" ) )
  {
    app.installTranslator( &inputTranslator );
    qDebug() <<  "Loaded input translation" << app.locale() << "for" << locale;
  }
  else
  {
    qDebug() <<  "Error in loading input translation for " << locale;
  }

  QString appBundleDir;
  QString demoDir;
#ifdef ANDROID
  appBundleDir = dataDir + "/qgis-data";
  demoDir = "assets:/demo-projects";
#endif
#ifdef Q_OS_IOS
  appBundleDir = QCoreApplication::applicationDirPath() + "/qgis-data";
  demoDir = QCoreApplication::applicationDirPath() + "/demo-projects";
#endif
#ifdef Q_OS_WIN32
  appBundleDir = QCoreApplication::applicationDirPath() + "\\qgis-data";
  //TODO win32 package demo projects
#endif
#ifdef Q_OS_LINUX
  appBundleDir = dataDir;
#endif
#ifdef Q_OS_MACOS
  appBundleDir = dataDir;
#endif
  InputProjUtils inputProjUtils;
  inputProjUtils.initProjLib( appBundleDir, dataDir, projectDir );
  init_qgis( appBundleDir );

  // AppSettings has to be initialized after QGIS app init (because of correct reading/writing QSettings).
  AppSettings as;
  bool hasLoadedDemoProjects = false;

  // copy demo projects when the app is launched for the first time
  if ( !as.demoProjectsCopied() )
  {
    copy_demo_projects( demoDir, projectDir );
    as.setDemoProjectsCopied( true );
    hasLoadedDemoProjects = true;
  }

  // Create Input classes
  AndroidUtils au;
  IosUtils iosUtils;
  LocalProjectsManager localProjectsManager( projectDir );
  MapThemesModel mtm;
  std::unique_ptr<MerginApi> ma =  std::unique_ptr<MerginApi>( new MerginApi( localProjectsManager ) );
  InputUtils iu( &au );
  MerginProjectStatusModel mpsm( localProjectsManager );
  InputHelp help( ma.get(), &iu );
  ProjectWizard pw( projectDir );

  // layer models
  LayersModel lm;
  LayersProxyModel browseLpm( &lm, LayerModelTypes::BrowseDataLayerSelection );
  LayersProxyModel recordingLpm( &lm, LayerModelTypes::ActiveLayerSelection );

  ActiveLayer al;
  ActiveProject activeProject( mtm, as, al, recordingLpm, localProjectsManager );
  std::unique_ptr<Purchasing> purchasing( new Purchasing( ma.get() ) );
  std::unique_ptr<VariablesManager> vm( new VariablesManager( ma.get() ) );
  vm->registerInputExpressionFunctions();

  SynchronizationManager syncManager( ma.get() );

  // build position kit, save active provider to QSettings and load previously active provider
  PositionKit pk;
  QObject::connect( &pk, &PositionKit::positionProviderChanged, &as, [&as]( AbstractPositionProvider * provider )
  {
    as.setActivePositionProviderId( provider ? provider->id() : QLatin1String() );
  } );
  pk.setPositionProvider( pk.constructActiveProvider( &as ) );

  // Lambda context object can be used in all lambda functions defined here,
  // it secures lambdas, so that they are destroyed when this object is destroyed to avoid crashes.
  QObject lambdaContext;

  QObject::connect( &app, &QGuiApplication::applicationStateChanged, &lambdaContext, []( Qt::ApplicationState state )
  {
    QString msg;

    // Instatiate QDebug with QString to redirect output to string
    // It is used to convert enum to string
    QDebug logHelper( &msg );

    logHelper << QStringLiteral( "Application changed state to:" ) << state;
    CoreUtils::log( QStringLiteral( "Input" ), msg );
  } );

  QObject::connect( &app, &QCoreApplication::aboutToQuit, &lambdaContext, []()
  {
    CoreUtils::log( QStringLiteral( "Input" ), QStringLiteral( "Application has quit" ) );
  } );

  QObject::connect( &activeProject, &ActiveProject::syncActiveProject, &syncManager, [&syncManager]( const LocalProject & project )
  {
    syncManager.syncProject( project, SyncOptions::Authorized, SyncOptions::Retry );
  } );

  // Direct connections
  QObject::connect( &app, &QGuiApplication::applicationStateChanged, &pk, &PositionKit::appStateChanged );
  QObject::connect( &pw, &ProjectWizard::projectCreated, &localProjectsManager, &LocalProjectsManager::addLocalProject );
  QObject::connect( &mtm, &MapThemesModel::mapThemeChanged, &recordingLpm, &LayersProxyModel::onMapThemeChanged );
  QObject::connect( &activeProject, &ActiveProject::projectReloaded, vm.get(), &VariablesManager::merginProjectChanged );
  QObject::connect( &activeProject, &ActiveProject::projectWillBeReloaded, &inputProjUtils, &InputProjUtils::resetHandlers );
  QObject::connect( &pw, &ProjectWizard::notify, &iu, &InputUtils::showNotificationRequested );
  QObject::connect( &iosUtils, &IosUtils::showToast, &iu, &InputUtils::showNotificationRequested );
  QObject::connect( &syncManager, &SynchronizationManager::syncFinished, &activeProject, [&activeProject]( const QString & projectFullName, bool successfully, int version, bool reloadNeeded )
  {
    if ( reloadNeeded && activeProject.projectFullName() == projectFullName )
    {
      activeProject.reloadProject( activeProject.qgsProject()->homePath() );
    }
  } );
  QObject::connect( QgsApplication::messageLog(),
                    static_cast<void ( QgsMessageLog::* )( const QString &message, const QString &tag, Qgis::MessageLevel level )>( &QgsMessageLog::messageReceived ),
                    &iu,
                    &InputUtils::onQgsLogMessageReceived );

  QFile projectLoadingFile( ActiveProject::LOADING_FLAG_FILE_PATH );
  if ( projectLoadingFile.exists() )
  {
    // Cleaning default project due to a project loading has crashed during the last run.
    as.setDefaultProject( QString() );
    projectLoadingFile.remove();
    CoreUtils::log( QStringLiteral( "Loading project error" ), QStringLiteral( "The Input has been unexpectedly finished during the last run." ) );
  }

#ifdef INPUT_TEST
  if ( tests.testingRequested() )
  {
    tests.initTestDeclarative();
    tests.init( ma.get(), purchasing.get(), &iu, vm.get(), &pk );
    return tests.runTest();
  }
#endif

  // we ship our fonts because they do not need to be installed on the target platform
  QStringList fonts;
  fonts << ":/Lato-Regular.ttf"
        << ":/Lato-Bold.ttf";
  for ( QString font : fonts )
  {
    if ( QFontDatabase::addApplicationFont( font ) == -1 )
      qDebug() << "!! Failed to load font" << font;
    else
      qDebug() << "Loaded font" << font;
  }
  app.setFont( QFont( "Lato" ) );

  QQmlEngine engine;
  addQmlImportPath( engine );
  initDeclarative();
  // QGIS environment variables to set
  // OGR_SQLITE_JOURNAL is set to DELETE to avoid working with WAL files
  // and properly close connection after writting changes to gpkg.
  qputenv( "OGR_SQLITE_JOURNAL", "DELETE" );

  // Register to QQmlEngine
  engine.rootContext()->setContextProperty( "__androidUtils", &au );
  engine.rootContext()->setContextProperty( "__iosUtils", &iosUtils );
  engine.rootContext()->setContextProperty( "__inputUtils", &iu );
  engine.rootContext()->setContextProperty( "__inputProjUtils", &inputProjUtils );
  engine.rootContext()->setContextProperty( "__inputHelp", &help );
  engine.rootContext()->setContextProperty( "__activeProject", &activeProject );
  engine.rootContext()->setContextProperty( "__syncManager", &syncManager );
  engine.rootContext()->setContextProperty( "__mapThemesModel", &mtm );
  engine.rootContext()->setContextProperty( "__appSettings", &as );
  engine.rootContext()->setContextProperty( "__merginApi", ma.get() );
  engine.rootContext()->setContextProperty( "__merginProjectStatusModel", &mpsm );
  engine.rootContext()->setContextProperty( "__recordingLayersModel", &recordingLpm );
  engine.rootContext()->setContextProperty( "__browseDataLayersModel", &browseLpm );
  engine.rootContext()->setContextProperty( "__activeLayer", &al );
  engine.rootContext()->setContextProperty( "__purchasing", purchasing.get() );
  engine.rootContext()->setContextProperty( "__projectWizard", &pw );
  engine.rootContext()->setContextProperty( "__localProjectsManager", &localProjectsManager );
  engine.rootContext()->setContextProperty( "__variablesManager", vm.get() );
  engine.rootContext()->setContextProperty( "__positionKit", &pk );

#ifdef HAVE_BLUETOOTH
  engine.rootContext()->setContextProperty( "__haveBluetooth", true );
#else
  engine.rootContext()->setContextProperty( "__haveBluetooth", false );
#endif

#ifdef MOBILE_OS
  engine.rootContext()->setContextProperty( "__appwindowvisibility", QWindow::Maximized );
  engine.rootContext()->setContextProperty( "__appwindowwidth", QVariant( 0 ) );
  engine.rootContext()->setContextProperty( "__appwindowheight", QVariant( 0 ) );
#else
  engine.rootContext()->setContextProperty( "__appwindowvisibility", QWindow::Windowed );
  engine.rootContext()->setContextProperty( "__appwindowwidth", 640 );
  engine.rootContext()->setContextProperty( "__appwindowheight", 1136 );
#endif
  engine.rootContext()->setContextProperty( "__version", version );

  // Even though enabling QT's HighDPI scaling removes the need to multiply pixel values with dp,
  // there are screens that need a "little help", because system DPR has different value than the
  // one we calculated. In these scenarios we use a ratio between real (our) DPR and DPR reported by QT.
  // Use `value * __dp` for each pixel value in QML
  engine.rootContext()->setContextProperty( "__dp", InputUtils::calculateDpRatio() );

  // Set simulated position for desktop builds
#ifdef DESKTOP_OS
  bool use_simulated_position = true;
#else
  bool use_simulated_position = false;
#endif
  engine.rootContext()->setContextProperty( "__use_simulated_position", use_simulated_position );

  QQmlComponent component( &engine, QUrl( "qrc:/main.qml" ) );
  QObject *object = component.create();

  if ( !component.errors().isEmpty() )
  {
    qDebug( "%s", QgsApplication::showSettings().toLocal8Bit().data() );

    qDebug() << "****************************************";
    qDebug() << "*****        QML errors:           *****";
    qDebug() << "****************************************";
    for ( const QQmlError &error : component.errors() )
    {
      qDebug() << "  " << error;
    }
    qDebug() << "****************************************";
    qDebug() << "****************************************";
  }

  if ( object == nullptr )
  {
    qDebug() << "FATAL ERROR: unable to create main.qml";
    return EXIT_FAILURE;
  }

#ifdef Q_OS_IOS
  QString logoUrl = "qrc:logo.png";
#else
  QString logoUrl = ":/logo.png";
#endif
  if ( QQuickWindow *quickWindow = qobject_cast<QQuickWindow *>( object ) )
  {
    quickWindow->setIcon( QIcon( logoUrl ) );
  }

#ifdef DESKTOP_OS
  QCommandLineParser parser;
  parser.addVersionOption();
  parser.process( app );
#endif

  // Add some data for debugging
  qDebug() << iu.dumpScreenInfo();
  qDebug() << "data directory: " << dataDir;
  qDebug() <<  "All up and running";

#ifdef ANDROID
  QtAndroid::hideSplashScreen();
#endif

  // Android scoped storage migration logic
#ifdef ANDROID
  QObject::connect( &au, &AndroidUtils::migrationFinished, [ &localProjectsManager, &as ]( bool success )
  {
    if ( success )
    {
      localProjectsManager.reloadDataDir();
    }
    as.setLegacyFolderMigrated( true );
  } );

  au.handleLegacyFolderMigration( &as, hasLoadedDemoProjects );
#endif

  // save app version to settings
  as.setAppVersion( version );

  int ret = EXIT_FAILURE;
  try
  {
    ret = app.exec();
  }
  catch ( QgsException &e )
  {
    CoreUtils::log( "Error", QStringLiteral( "Caught unhandled QgsException %1" ).arg( e.what() ) );
  }
  catch ( std::exception &e )
  {
    CoreUtils::log( "Error", QStringLiteral( "Caught unhandled std::exception %1" ).arg( e.what() ) );
  }
  catch ( ... )
  {
    CoreUtils::log( "Error", QStringLiteral( "Caught unhandled unknown exception" ) );
  }
  return ret;
}


