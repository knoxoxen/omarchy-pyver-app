import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: win
    width: 640
    height: 560
    minimumWidth: 460
    minimumHeight: 380
    visible: true
    title: qsTr("Python versions")
    color: "#0e0e10"

    Material.theme: Material.Dark
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    property bool helpVisible: false
    property var versions: []
    property int selectedIndex: -1
    property bool indicatorMode: false

    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; onActivated: win.helpVisible = !win.helpVisible }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }
    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; autoRepeat: false; enabled: !win.helpVisible; onActivated: win.useSelected() }
    Shortcut { sequence: "Up"; context: Qt.ApplicationShortcut; enabled: !win.helpVisible; onActivated: win.moveSelection(-1) }
    Shortcut { sequence: "Down"; context: Qt.ApplicationShortcut; enabled: !win.helpVisible; onActivated: win.moveSelection(1) }

    function useSelected() {
        if (win.versions.length === 0)
            return;
        let index = win.selectedIndex;
        if (index < 0 || index >= win.versions.length)
            index = 0;
        backend.useVersion(win.versions[index].version);
    }
    function moveSelection(delta) {
        if (win.versions.length === 0)
            return;
        let index = win.selectedIndex + delta;
        if (index < 0)
            index = 0;
        if (index > win.versions.length - 1)
            index = win.versions.length - 1;
        win.selectedIndex = index;
    }
    function refreshVersions() {
        win.versions = backend.versionEntries();
        win.selectedIndex = -1;
        const current = backend.defaultVersion;
        for (let i = 0; i < win.versions.length; ++i) {
            if (win.versions[i].version === current) {
                win.selectedIndex = i;
                break;
            }
        }
    }

    Connections {
        target: backend
        function onVersionsChanged() { win.refreshVersions() }
    }

    Item {
        anchors.fill: parent
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                Label { text: qsTr("Installed Python versions"); font.pixelSize: 18; font.bold: true; color: Material.text; Layout.fillWidth: true; elide: Text.ElideRight }
                Label { text: qsTr("Default") + ": " + backend.defaultVersion; font.pixelSize: 14; color: Material.text }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                ListView {
                    id: versionList
                    anchors.fill: parent
                    clip: true
                    model: win.versions
                    spacing: 4
                    delegate: Item {
                        id: row
                        required property var modelData
                        width: ListView.view.width
                        height: 52
                        readonly property bool isCurrent: row.modelData.current === true
                        Rectangle {
                            anchors.fill: parent
                            radius: Math.min(6, theme.radius)
                            color: rowMouse.containsMouse ? "#1a1a1d" : (row.isCurrent ? "#26262b" : "transparent")
                        }
                        MouseArea {
                            id: rowMouse
                            anchors.fill: parent
                            preventStealing: true
                            onClicked: backend.useVersion(row.modelData.version)
                        }
                        RowLayout {
                            anchors.fill: parent
                            Layout.fillWidth: true
                            Layout.leftMargin: 14
                            Layout.rightMargin: 14
                            spacing: 10
                            Text { text: "Python " + row.modelData.version; font.pixelSize: 16; color: Material.text; font.bold: row.isCurrent }
                            Text { text: row.modelData.source; font.pixelSize: 12; color: Material.disabledText }
                            Item { Layout.fillWidth: true }
                            Rectangle { visible: row.isCurrent; width: 10; height: 10; radius: 5; color: Material.accent }
                        }
                    }
                }
                Label { anchors.centerIn: parent; visible: win.versions.length === 0; text: qsTr("No Python versions found"); font.pixelSize: 14; color: Material.disabledText; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; width: Math.min(parent.width, 320) }
            }

            RowLayout {
                Layout.fillWidth: true
                Button { id: refreshButton; text: qsTr("Refresh"); onClicked: backend.refresh() }
                Label { text: backend.statusMessage; Layout.fillWidth: true; font.pixelSize: 12; color: Material.disabledText; elide: Text.ElideRight; wrapMode: Text.WordWrap }
            }
        }
    }

    Popup {
        id: helpPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.4) }
        background: Rectangle { color: Material.background; border.color: Material.border; border.width: 1; radius: Math.min(8, theme.radius) }
        contentItem: ColumnLayout {
            spacing: 18
            Label { text: qsTr("Keyboard shortcuts"); font.pixelSize: 16; font.bold: true; color: Material.text }
            ColumnLayout {
                spacing: 10
                Repeater {
                    model: [[qsTr("?"), qsTr("Show or hide this help")], [qsTr("Space"), qsTr("Use the selected version")], [qsTr("Up"), qsTr("Select the previous version")], [qsTr("Down"), qsTr("Select the next version")], [qsTr("Q"), qsTr("Quit")]]
                    RowLayout { required property var modelData; spacing: 14; Label { text: modelData[0]; font.pixelSize: 12; color: Material.text; Layout.preferredWidth: 110 } Label { text: modelData[1]; font.pixelSize: 13; color: Material.disabledText } }
                }
            }
            Button { text: qsTr("Close"); onClicked: helpPopup.close() }
        }
    }
}