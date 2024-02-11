import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0

import "../Controls2"
import "../Controls2/TextTypes"


ListView {
    id: menuContent

    property var rootWidth

    width: rootWidth
    height: menuContent.contentItem.height

    clip: true
    interactive: false

    ButtonGroup {
        id: containersRadioButtonGroup
    }

//    Connections {
//        target: ServersModel

//        function onCurrentlyProcessedServerIndexChanged() {
//            if (ContainersModel.getDefaultContainer()) {
//                menuContent.checkCurrentItem()
//            }
//        }
//    }

    function checkCurrentItem() {
        var item = menuContent.itemAtIndex(currentIndex)
        if (item !== null) {
            var radioButton = item.children[0].children[0]
            radioButton.checked = true
        }
    }

    delegate: Item {
        implicitWidth: rootWidth
        implicitHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.fill: parent
            anchors.rightMargin: 16
            anchors.leftMargin: 16

            VerticalRadioButton {
                id: containerRadioButton

                Layout.fillWidth: true

                text: name
                descriptionText: description

                ButtonGroup.group: containersRadioButtonGroup

                imageSource: "qrc:/images/controls/download.svg"
                showImage: !isInstalled

                checkable: isInstalled && !ConnectionController.isConnected && isSupported
                checked: proxyContainersModel.mapToSource(index) === ServersModel.getDefaultContainer()

                onClicked: {
                    if (ConnectionController.isConnected && isInstalled) {
                        PageController.showNotificationMessage(qsTr("Unable change protocol while there is an active connection"))
                        return
                    }

                    if (checked) {
                        containersDropDown.menuVisible = false
                        ServersModel.setDefaultContainer(proxyContainersModel.mapToSource(index))
                    } else {
                        if (!isSupported && isInstalled) {
                            PageController.showErrorMessage(qsTr("The selected protocol is not supported on the current platform"))
                            return
                        }

                        ContainersModel.setCurrentlyProcessedContainerIndex(proxyContainersModel.mapToSource(index))
                        InstallController.setShouldCreateServer(false)
                        PageController.goToPage(PageEnum.PageSetupWizardProtocolSettings)
                        containersDropDown.menuVisible = false
                    }
                }

                MouseArea {
                    anchors.fill: containerRadioButton
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }

            DividerType {
                Layout.fillWidth: true
            }
        }
    }
}
