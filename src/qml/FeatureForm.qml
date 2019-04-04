import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls 1.4 as Controls
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import QtQml.Models 2.11
import QtQml 2.3

import org.qgis 1.0
import org.qfield 1.0
import "js/style.js" as Style
import QtQuick.Controls.Styles 1.4
import "."

Page {
  signal saved
  signal cancelled
  signal aboutToSave

  property AttributeFormModel model
  property alias toolbarVisible: toolbar.visible
  property bool embedded: false

  function reset() {
    master.reset()
  }

  id: form

  states: [
    State {
      name: 'ReadOnly'
    },
    State {
      name: 'Edit'
    },
    State {
      name: 'Add'
    }
  ]

  /**
   * a substate used under 'Add' (not yet under 'Edit' but possibly in future)
   * in case that the feature needs to be stored "meanwhile"
   * e.g. on relation editor widget when adding childs to a not yet stored parent
   */
  property bool buffered: false

  /**
   * This is a relay to forward private signals to internal components.
   */
  QtObject {
    id: master

    /**
     * This signal is emitted whenever the state of Flickables and TabBars should
     * be restored.
     */
    signal reset
  }

  Item {
    id: container

    clip: true

    anchors {
      top: toolbar.bottom
      bottom: parent.bottom
      left: parent.left
      right: parent.right
    }

    Flickable {
      id: flickable
      anchors {
        left: parent.left
        right: parent.right
      }
      height: tabRow.height

      flickableDirection: Flickable.HorizontalFlick
      contentWidth: tabRow.width

      // Tabs
      TabBar {
        id: tabRow
        visible: model.hasTabs
        height: 48 * dp

        Connections {
          target: master
          onReset: tabRow.currentIndex = 0
        }

        Connections {
          target: swipeView
          onCurrentIndexChanged: tabRow.currentIndex = swipeView.currentIndex
        }

        Repeater {
          model: form.model && form.model.hasTabs ? form.model : 0

          TabButton {
            id: tabButton
            text: Name
            leftPadding: 8 * dp
            rightPadding: 8 * dp

            width: contentItem.width + leftPadding + rightPadding
            height: 48 * dp

            contentItem: Text {
              // Make sure the width is derived from the text so we can get wider
              // than the parent item and the Flickable is useful
              width: paintedWidth
              text: tabButton.text
              // color: tabButton.down ? "#17a81a" : "#21be2b"
              color: !tabButton.enabled ? "#999999" : tabButton.down ||
                                        tabButton.checked ? "#1B5E20" : "#4CAF50"
              font.weight: tabButton.checked ? Font.DemiBold : Font.Normal

              horizontalAlignment: Text.AlignHCenter
              verticalAlignment: Text.AlignVCenter
            }
          }
        }
      }
    }

    SwipeView {
      id: swipeView
      currentIndex: tabRow.currentIndex
      anchors {
        top: flickable.bottom
        left: parent.left
        right: parent.right
        bottom: parent.bottom
      }

      Repeater {
        // One page per tab in tabbed forms, 1 page in auto forms
        model: form.model.hasTabs ? form.model : 1

        Item {
          id: formPage
          property int currentIndex: index

          Rectangle {
            anchors.fill: swipeView
            color: "white"
          }

          /**
           * The main form content area
           */
          ListView {
            id: content
            anchors.fill: parent
            clip: true
            section.property: "Group"
            section.labelPositioning: ViewSection.CurrentLabelAtStart | ViewSection.InlineLabels
            section.delegate: Component {
              // section header: group box name
              Rectangle {
                width: parent.width
                height: section === "" ? 0 : 30 * dp
                color: "lightGray"

                Text {
                  anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }
                  font.bold: true
                  text: section
                }
              }
            }

            Connections {
              target: master
              onReset: content.contentY = 0
            }

            model: SubModel {
              id: contentModel
              model: form.model
              rootIndex: form.model && form.model.hasTabs ? form.model.index(currentIndex, 0) : undefined
            }

            delegate: fieldItem
          }
        }
      }
    }
  }

  /**
   * A field editor
   */
  Component {
    id: fieldItem

    Item {
      id: fieldContainer
      visible: Type === 'field' || Type === 'relation'
      height: childrenRect.height

      anchors {
        left: parent.left
        right: parent.right
        leftMargin: 12 * dp
      }

      Controls.Label {
        id: fieldLabel

        text: Name || ''
        font.bold: true
        color: ConstraintValid ? "black" : "#c0392b"
      }

      Controls.Label {
        id: constraintDescriptionLabel
        anchors {
          left: parent.left
          right: parent.right
          top: fieldLabel.bottom
        }

        text: ConstraintDescription || ''
        height: ConstraintValid ? 0 : undefined
        visible: !ConstraintValid

        color: "#e67e22"
      }

      Item {
        id: placeholder
        height: childrenRect.height
        anchors { left: parent.left; right: rememberCheckbox.left; top: constraintDescriptionLabel.bottom }

        Loader {
          id: attributeEditorLoader

          signal bufferFeature

          height: childrenRect.height
          anchors { left: parent.left; right: parent.right }

          enabled: (form.state !== 'ReadOnly' || EditorWidget === 'RelationEditor') && !!AttributeEditable
          property bool readOnly: form.state === 'ReadOnly' || embedded && EditorWidget === 'RelationEditor'
          property var value: AttributeValue
          property var config: ( EditorWidgetConfig || {} )
          property var widget: EditorWidget
          property var field: Field
          property var fieldType: FieldType
          property var relationId: RelationId
          property var associatedRelationId: AssociatedRelationId
          property var constraintValid: ConstraintValid
          property var currentFeature: form.model.featureModel.feature

          active: widget !== 'Hidden'
          source: 'editorwidgets/' + ( widget || 'TextEdit' ) + '.qml'

          onStatusChanged: {
            if ( attributeEditorLoader.status === Loader.Error )
            {
              console.warn( "Editor widget type '" + EditorWidget + "' not avaliable." )
              source = 'editorwidgets/TextEdit.qml'
            }
          }

          onBufferFeature: {
            buffer()
          }
        }

        Connections {
          target: form
          onAboutToSave: {
            try {
              attributeEditorLoader.item.pushChanges()
            }
            catch ( err )
            {}
          }
        }

        Connections {
          target: attributeEditorLoader.item
          onValueChanged: {
            AttributeValue = isNull ? undefined : value
          }
        }
      }

      Controls.CheckBox {
        id: rememberCheckbox
        checked: RememberValue ? true : false

        visible: form.state === 'Add' && EditorWidget !== "Hidden"
        width: visible ? undefined : 0

        anchors { right: parent.right; top: fieldLabel.bottom }

        onCheckedChanged: {
          RememberValue = checked
        }
      }
    }
  }

  function save() {
    //if this is for some reason not handled before (like when tiping on a map while editing)
    if( !model.constraintsValid ) {
        displayToast( "Constraints not valid - cancel editing" )
        cancelled()
        return
    }

    parent.focus = true
    aboutToSave()

    if ( form.state === 'Add' && !buffered ) {
      model.create()
      state = 'Edit'
    }
    else
    {
      model.save()
    }

    saved()
  }

  function buffer(){
      aboutToSave() //used the same way like on save

      if ( form.state === 'Add' && !buffered ) {
        //model.create()
        buffered = true
      }
      else
      {
      //  model.save()
      }

      //evtl. buffered()
  }

  Connections {
    target: Qt.inputMethod
    onVisibleChanged: {
      Qt.inputMethod.commit()
    }
  }

  /** The title toolbar **/
  ToolBar {
    id: toolbar
    height: visible ? 48 * dp : 0
    visible: form.state === 'Add'

    anchors {
      top: parent.top
      left: parent.left
      right: parent.right
    }

    background: Rectangle {
      //testwise have special color for buffered
      color: model.constraintsValid ?  form.state === 'Add' ? form.buffered ? "hotpink" : "blue" : "#80CC28" : "orange"
    }

    RowLayout {
      anchors.fill: parent
      Layout.margins: 0

      Button {
        id: saveButton

        Layout.alignment: Qt.AlignTop | Qt.AlignLeft

        visible: form.state === 'Add' || form.state === 'Edit'
        width: 48*dp
        height: 48*dp
        clip: true
        bgcolor: "#212121"

        iconSource: Style.getThemeIcon( "ic_check_white_48dp" )

        onClicked: {
          if( model.constraintsValid ) {
            save()
          } else {
            displayToast( "Constraints not valid" )
          }
        }
      }

      Label {
        id: titleLabel

        text:
        {
          var currentLayer = model.featureModel.currentLayer
          var layerName = 'N/A'
          if (currentLayer !== null)
            layerName = currentLayer.name

          if ( form.state === 'Add' )
            qsTr( 'Add feature on <i>%1</i>' ).arg(layerName )
          else if ( form.state === 'Edit' )
            qsTr( 'Edit feature on <i>%1</i>' ).arg(layerName)
          else
            qsTr( 'View feature on <i>%1</i>' ).arg(layerName)
        }
        font.bold: true
        font.pointSize: 16
        elide: Label.ElideRight
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        Layout.fillWidth: true
      }

      Button {
        id: closeButton

        Layout.alignment: Qt.AlignTop | Qt.AlignRight

        width: 48*dp
        height: 48*dp
        clip: true
        bgcolor: "#212121"

        iconSource: form.state === 'Add' ? Style.getThemeIcon( "ic_delete_forever_white_24dp" ) : Style.getThemeIcon( "ic_close_white_24dp" )

        onClicked: {
          Qt.inputMethod.hide()
          cancelled()
        }
      }
    }
  }
}
