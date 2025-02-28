/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "featuresmodel.h"
#include "coreutils.h"

#include "qgsproject.h"
#include "qgsexpressioncontextutils.h"

FeaturesModel::FeaturesModel( QObject *parent )
  : QAbstractListModel( parent ),
    mLayer( nullptr )
{
}

FeaturesModel::~FeaturesModel() = default;

void FeaturesModel::populate()
{
  if ( mLayer )
  {
    beginResetModel();
    mFeatures.clear();

    QgsFeatureRequest req;
    setupFeatureRequest( req );

    QgsFeatureIterator it = mLayer->getFeatures( req );
    QgsFeature f;

    while ( it.nextFeature( f ) )
    {
      mFeatures << FeatureLayerPair( f, mLayer );
    }

    endResetModel();
  }
}

void FeaturesModel::setup()
{
  // define in submodels
}

QVariant FeaturesModel::data( const QModelIndex &index, int role ) const
{
  int row = index.row();
  if ( row < 0 || row >= mFeatures.count() )
    return QVariant();

  if ( !index.isValid() )
    return QVariant();

  const FeatureLayerPair pair = mFeatures.at( index.row() );

  switch ( role )
  {
    case FeatureTitle: return featureTitle( pair );
    case FeatureId: return QVariant( pair.feature().id() );
    case Feature: return QVariant::fromValue<QgsFeature>( pair.feature() );
    case FeaturePair: return QVariant::fromValue<FeatureLayerPair>( pair );
    case Description: return QVariant( QString( "Feature ID %1" ).arg( pair.feature().id() ) );
    case SearchResult: return searchResultPair( pair );
    case Qt::DisplayRole: return featureTitle( pair );
  }

  return QVariant();
}

int FeaturesModel::rowCount( const QModelIndex &parent ) const
{
  // For list models only the root node (an invalid parent) should return the list's size. For all
  // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
  if ( parent.isValid() )
    return 0;

  return mFeatures.count();
}

QVariant FeaturesModel::featureTitle( const FeatureLayerPair &featurePair ) const
{
  QString title;

  QgsExpressionContext context( QgsExpressionContextUtils::globalProjectLayerScopes( featurePair.layer() ) );
  context.setFeature( featurePair.feature() );
  QgsExpression expr( featurePair.layer()->displayExpression() );
  title = expr.evaluate( &context ).toString();

  if ( title.isEmpty() )
    return featurePair.feature().id();

  return title;
}

QString FeaturesModel::searchResultPair( const FeatureLayerPair &pair ) const
{
  if ( mSearchExpression.isEmpty() )
    return QString();

  QgsFields fields = pair.feature().fields();
  QStringList words = mSearchExpression.split( ' ', QString::SplitBehavior::SkipEmptyParts );
  QStringList foundPairs;

  for ( const QString &word : words )
  {
    for ( const QgsField &field : fields )
    {
      if ( field.configurationFlags().testFlag( QgsField::ConfigurationFlag::NotSearchable ) )
        continue;

      QString attrValue = pair.feature().attribute( field.name() ).toString();

      if ( attrValue.toLower().indexOf( word.toLower() ) != -1 )
      {
        foundPairs << field.name() + ": " + attrValue;

        // remove found field from list of fields to not select it more than once
        fields.remove( fields.lookupField( field.name() ) );
      }
    }
  }

  return foundPairs.join( ", " );
}

QString FeaturesModel::buildSearchExpression()
{
  if ( mSearchExpression.isEmpty() || !mLayer )
    return QString();

  const QgsFields fields = mLayer->fields();
  QStringList expressionParts;
  QStringList wordExpressions;

  QStringList words = mSearchExpression.split( ' ', QString::SplitBehavior::SkipEmptyParts );

  for ( const QString &word : words )
  {
    bool searchExpressionIsNumeric;
    int filterInt = word.toInt( &searchExpressionIsNumeric );
    Q_UNUSED( filterInt ); // we only need to know if expression is numeric, int value is not used


    for ( const QgsField &field : fields )
    {
      if ( field.configurationFlags().testFlag( QgsField::ConfigurationFlag::NotSearchable ) )
        continue;

      if ( field.isNumeric() && searchExpressionIsNumeric )
        expressionParts << QStringLiteral( "%1 ~ '%2.*'" ).arg( QgsExpression::quotedColumnRef( field.name() ), word );
      else if ( field.type() == QVariant::String )
        expressionParts << QStringLiteral( "%1 ILIKE '%%2%'" ).arg( QgsExpression::quotedColumnRef( field.name() ), word );
    }
    wordExpressions << QStringLiteral( "(%1)" ).arg( expressionParts.join( QLatin1String( " ) OR ( " ) ) );
    expressionParts.clear();
  }

  QString expression = QStringLiteral( "(%1)" ).arg( wordExpressions.join( QLatin1String( " ) AND ( " ) ) );

  return expression;
}

void FeaturesModel::setupFeatureRequest( QgsFeatureRequest &request )
{
  if ( !mSearchExpression.isEmpty() )
  {
    request.setFilterExpression( buildSearchExpression() );
  }

  request.setLimit( FEATURES_LIMIT );
}

void FeaturesModel::reloadFeatures()
{
  populate();
}

int FeaturesModel::layerFeaturesCount() const
{
  if ( mLayer && mLayer->isValid() )
  {
    return mLayer->dataProvider()->featureCount();
  }

  return 0;
}

QHash<int, QByteArray> FeaturesModel::roleNames() const
{
  QHash<int, QByteArray> roleNames = QAbstractListModel::roleNames();
  roleNames[FeatureTitle] = QStringLiteral( "FeatureTitle" ).toLatin1();
  roleNames[FeatureId] = QStringLiteral( "FeatureId" ).toLatin1();
  roleNames[Feature] = QStringLiteral( "Feature" ).toLatin1();
  roleNames[FeaturePair] = QStringLiteral( "FeaturePair" ).toLatin1();
  roleNames[Description] = QStringLiteral( "Description" ).toLatin1();
  roleNames[SearchResult] = QStringLiteral( "SearchResult" ).toLatin1();
  return roleNames;
}

int FeaturesModel::rowFromRoleValue( const int role, const QVariant &value ) const
{
  for ( int i = 0; i < mFeatures.count(); ++i )
  {
    QVariant d = data( index( i, 0 ), role );
    if ( d == value )
    {
      return i;
    }
  }
  return -1;
}

QVariant FeaturesModel::convertRoleValue( const int role, const QVariant &value, const int requestedRole ) const
{
  for ( int i = 0; i < mFeatures.count(); ++i )
  {
    QVariant d = data( index( i, 0 ), role );
    if ( d.toString().trimmed() == value.toString().trimmed() )
    {
      QVariant key = data( index( i, 0 ), requestedRole );
      return key;
    }
  }
  return QVariant();
}

void FeaturesModel::reset()
{
  mFeatures.clear();
  mLayer = nullptr;
  mSearchExpression.clear();
}

QString FeaturesModel::searchExpression() const
{
  return mSearchExpression;
}

void FeaturesModel::setSearchExpression( const QString &searchExpression )
{
  if ( mSearchExpression != searchExpression )
  {
    mSearchExpression = searchExpression;
    emit searchExpressionChanged( mSearchExpression );

    populate();
  }
}

int FeaturesModel::featuresLimit() const
{
  return FEATURES_LIMIT;
}

void FeaturesModel::setLayer( QgsVectorLayer *newLayer )
{
  if ( mLayer != newLayer )
  {
    if ( mLayer )
    {
      disconnect( mLayer, &QgsMapLayer::willBeDeleted, this, &FeaturesModel::reset );
    }

    mLayer = newLayer;
    emit layerChanged( mLayer );

    if ( mLayer )
    {
      // avoid dangling pointers to mLayer when switching projects
      connect( mLayer, &QgsMapLayer::willBeDeleted, this, &FeaturesModel::reset );
    }
  }
}

QgsVectorLayer *FeaturesModel::layer() const
{
  return mLayer;
}
