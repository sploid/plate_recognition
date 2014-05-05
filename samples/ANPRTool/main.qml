import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.1
import OpenCV.Controls 1.0

Window
{
	visible: true
	width: 360
	height: 360

	CameraItem
	{
		id: cameraItem
		anchors.fill: parent
		focus : visible // to receive focus and capture key events when visible
		Button
		{
			id: cameraButton
			anchors.right: parent.right
			anchors.top: parent.top
			anchors.topMargin: 20.0
			anchors.rightMargin: 20.0
			iconSource: "qrc:///camera.ico"
		}

		Button
		{
			id: settingsButton
			anchors.right: parent.right
			anchors.top: cameraButton.bottom
			anchors.topMargin: 20.0
			anchors.rightMargin: 20.0
			iconSource: "qrc:///settings.ico"
		}
	}


/*	OrientationSensor
	{
		id: orientation
		active: true

		onReadingChanged:
		{
			if ( reading.orientation == OrientationReading.TopUp )
				console.log( "!!! TopUp !!!" );
			else if ( reading.orientation == OrientationReading.TopDown )
				console.log( "!!! TopDown !!!" );
			else if ( reading.orientation == OrientationReading.LeftUp )
				console.log( "!!! LeftUp !!!" );
			else if ( reading.orientation == OrientationReading.RightUp )
				console.log( "!!! RightUp !!!" );
			else if ( reading.orientation == OrientationReading.FaceUp )
				console.log( "!!! FaceUp !!!" );
			else if ( reading.orientation == OrientationReading.FaceDown )
				console.log( "!!! FaceDown !!!" );
			else
				console.log( "!!! HZ !!!" );
		}
	}*/
}
