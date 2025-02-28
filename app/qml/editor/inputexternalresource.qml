/***************************************************************************
 externalresource.qml
  --------------------------------------
  Date                 : 2017
  Copyright            : (C) 2017 by Matthias Kuhn
  Email                : matthias@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick 2.5
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3

import "../components"

/**
 * External Resource (Photo capture) for QGIS Attribute Form
 * Requires various global properties set to function, see featureform Loader section
 * Do not use directly from Application QML
 * The widget is interactive which allows interactions even in readOnly state (e.g showing preview), but no edit!
 *
 * Overview of path handling
 * -------------------------
 *
 * Project home path: comes from QgsProject::homePath() - by default it points to the folder where
 * the .qgs/.qgz project file is stored, but can be changed manually by the user.
 *
 * Default path: defined in the field's configuration. This is the path where newly captured images will be stored.
 * It has to be an absolute path. It can be defined as an expression (e.g. @project_home || '/photos') or
 * by plain path (e.g. /home/john/photos). If not defined, project home path is used.
 *
 * In the field's configuration, there are three ways how path to pictures is stored in field values:
 * absolute paths, relative to default path and relative to project path. Below is an example of how
 * the final field values of paths are calculated.
 *
 *   variable         |     value
 * -------------------+--------------------------------
 * project home path  |  /home/john
 * default path       |  /home/john/photos
 * image path         |  /home/john/photos/img0001.jpg
 *
 *
 *    storage type          |   calculation of field value    |      final field value
 * -------------------------+---------------------------------+--------------------------------
 * absolute path            |  image path                     |   /home/john/photos/img0001.jpg
 * relative to default path |  image path - default path      |   img0001.jpg
 * relative to project path |  image path - project home path |   photos/img0001.jpg
 */
Item {
  signal editorValueChanged(var newValue, bool isNull)

  property var image: image
  property var cameraIcon: customStyle.icons.camera
  property var deleteIcon: customStyle.icons.remove
  property var galleryIcon: customStyle.icons.gallery
  property var backIcon: customStyle.icons.back
  property real iconSize:  customStyle.fields.height
  property real textMargin: 10 * __dp
  /**
   * 0 - Relative path disabled
   * 1 - Relative path to project
   * 2 - Relative path to defaultRoot defined in the config - Default path field in the widget configuration form
   */
  property int relativeStorageMode: config["RelativeStorage"]

  /**
   * This evaluates the "default path" with the following order:
   * 1. evaluate default path expression if defined,
   * 2. use default path value if not empty,
   * 3. use project home folder
   */
  property string targetDir: __inputUtils.resolveTargetDir(homePath, config, featurePair, activeProject)

  property string prefixToRelativePath: __inputUtils.resolvePrefixForRelativePath(relativeStorageMode, homePath, targetDir)

  // Meant to be use with the save callback - stores image source
  property string sourceToDelete

  function callbackOnSave() {
    externalResourceHandler.onFormSave(fieldItem)
  }
  function callbackOnCancel() {
    externalResourceHandler.onFormCanceled(fieldItem)
  }

  function showDefaultPanel() {
    if (!photoCapturePanelLoader.item) {
      // Load the photo capture panel if not loaded yet
      photoCapturePanelLoader.setSource("qrc:/PhotoPanel.qml")
      photoCapturePanelLoader.item.height = window.height
      photoCapturePanelLoader.item.width = window.width
      photoCapturePanelLoader.item.edge = Qt.RightEdge
      photoCapturePanelLoader.item.imageButtonSize = fieldItem.iconSize
      photoCapturePanelLoader.item.backButtonSource = fieldItem.backIcon
    }
    photoCapturePanelLoader.item.visible = true
    photoCapturePanelLoader.item.targetDir = targetDir
    photoCapturePanelLoader.item.prefixToRelativePath = prefixToRelativePath
    photoCapturePanelLoader.item.fieldItem = fieldItem
  }

  id: fieldItem
  enabled: true // its interactive widget
  height: customStyle.fields.height * 3
  anchors {
    left: parent.left
    right: parent.right
  }

  states: [
    State {
      name: "valid"
    },
    State {
      name: "notSet"
    },
    State {
      name: "notAvailable"
    }
  ]

  Loader {
    id: photoCapturePanelLoader
  }

  Connections {
    target: photoCapturePanelLoader.item
    onConfirmButtonClicked: externalResourceHandler.confirmImage(fieldItem, path, filename)
  }

  Rectangle {
    id: imageContainer
    width: parent.width
    height: parent.height
    color: customStyle.fields.backgroundColor
    radius: customStyle.fields.cornerRadius

    Image {
      property var currentValue: value

      id: image
      height: imageContainer.height
      sourceSize.height: imageContainer.height
      autoTransform: true
      fillMode: Image.PreserveAspectFit
      visible: fieldItem.state === "valid"
      anchors.verticalCenter: parent.verticalCenter
      anchors.horizontalCenter: parent.horizontalCenter

      MouseArea {
        anchors.fill: parent
        onClicked: externalResourceHandler.previewImage( __inputUtils.getAbsolutePath( image.currentValue, prefixToRelativePath ) )
      }

      onCurrentValueChanged: {
        image.source = image.getSource()
      }

      function getSource() {
        var absolutePath = __inputUtils.getAbsolutePath( image.currentValue, prefixToRelativePath )
        if (image.status === Image.Error) {
          fieldItem.state = "notAvailable"
          return ""
        }
        else if (image.currentValue && __inputUtils.fileExists(absolutePath)) {
          fieldItem.state = "valid"
          return "file://" + absolutePath
        }
        else if (!image.currentValue) {
          fieldItem.state = "notSet"
          return ""
        }
        fieldItem.state = "notAvailable"
        return "file://" + absolutePath
      }
    }
  }

  Button {
    id: deleteButton
    visible: !readOnly && fieldItem.state !== "notSet"
    width: buttonsContainer.itemHeight
    height: width
    padding: 0

    anchors.right: imageContainer.right
    anchors.bottom: imageContainer.bottom
    anchors.margins: buttonsContainer.itemHeight/4

    onClicked: externalResourceHandler.removeImage( fieldItem, __inputUtils.getAbsolutePath( image.currentValue, prefixToRelativePath ) )

    background: Image {
      id: deleteIcon
      source: fieldItem.deleteIcon
      width: deleteButton.width
      height: deleteButton.height
      sourceSize.width: width
      sourceSize.height: height
      fillMode: Image.PreserveAspectFit
    }

    ColorOverlay {
      anchors.fill: deleteIcon
      source: deleteIcon
      color: customStyle.fields.attentionColor
    }
  }

  Item {
    property real itemHeight: fieldItem.height * 0.2

    id: buttonsContainer
    anchors.centerIn: imageContainer
    anchors.fill: imageContainer
    anchors.margins: fieldItem.textMargin
    visible: fieldItem.state === "notSet"

    RowLayout {
      anchors.fill: parent

      IconTextItem {
        id: photoButton
        fontColor: customStyle.fields.fontColor
        fontPixelSize: customStyle.fields.fontPixelSize
        iconSource: fieldItem.cameraIcon
        iconSize: buttonsContainer.itemHeight
        labelText: qsTr("Take a photo")
        visible: !readOnly && fieldItem.state !== " valid"

        Layout.preferredHeight: parent.height
        Layout.fillWidth: true
        Layout.preferredWidth: ( parent.width - lineContainer.width ) / 2

        MouseArea {
          anchors.fill: parent
          onClicked: {
            if (externalResourceHandler.capturePhoto) {
              externalResourceHandler.capturePhoto(fieldItem)
            } else {
              showDefaultPanel()
            }
          }
        }
      }

      Item {
        id: lineContainer
        visible: !readOnly && fieldItem.state !== " valid"
        Layout.fillWidth: true
        Layout.preferredHeight: parent.height
        Layout.preferredWidth: line.width * 2

        Rectangle {
          id: line

          height: parent.height * 0.7
          color: customStyle.fields.fontColor
          width: 1.5 * __dp
          anchors.centerIn: parent
        }
      }

      IconTextItem {
        id: browseButton
        fontColor: customStyle.fields.fontColor
        fontPixelSize: customStyle.fields.fontPixelSize
        iconSource: fieldItem.galleryIcon
        iconSize: buttonsContainer.itemHeight
        labelText: qsTr("From gallery")

        visible: !readOnly && fieldItem.state !== " valid"

        Layout.preferredHeight: parent.height
        Layout.fillWidth: true
        Layout.preferredWidth: ( parent.width - lineContainer.width ) / 2

        MouseArea {
          anchors.fill: parent
          onClicked: externalResourceHandler.chooseImage(fieldItem)
        }
      }
    }
  }

  Text {
    id: text
    height: parent.height
    width: imageContainer.width - 2* fieldItem.textMargin
    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
    text: qsTr("Image is not available: ") + image.currentValue
    font.pixelSize: customStyle.fields.fontPixelSize
    color: customStyle.fields.fontColor
    anchors.leftMargin: buttonsContainer.itemHeight + fieldItem.textMargin
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    elide: Text.ElideRight
    visible: fieldItem.state === "notAvailable"
  }

}
