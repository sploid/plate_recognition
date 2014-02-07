package ru.sploid.platerecog;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewFrame;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.Point;
import org.opencv.core.Scalar;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewListener2;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

public class RecogActivity extends Activity implements CvCameraViewListener2 {
    private static final String    TAG = "OCVSample::Activity";

    private CameraBridgeViewBase   m_open_cv_camera_view;
    
    private static final int VIEW_MODE_DRAW = 0;
    private static final int VIEW_MODE_RECOG = 1;
    private static final int VIEW_MODE_SAVE = 2;

    private int m_view_mode = VIEW_MODE_DRAW;
    private Mat m_rgba;
    private double m_last_ticks = 0.;
    

    private BaseLoaderCallback  mLoaderCallback = new BaseLoaderCallback( this )
    {
        @Override
        public void onManagerConnected( int status )
        {
            switch ( status )
            {
                case LoaderCallbackInterface.SUCCESS:
                {
                    Log.i(TAG, "OpenCV loaded successfully");

                    // Load native library after(!) OpenCV initialization
                    System.loadLibrary("recog_sample");

                    m_open_cv_camera_view.enableView();
                }
                break;
                default:
                {
                    super.onManagerConnected(status);
                }
                break;
            }
        }
    };

    public RecogActivity()
    {
        Log.i( TAG, "Instantiated new " + this.getClass() );
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        Log.i( TAG, "called onCreate" );
        super.onCreate( savedInstanceState );
        getWindow().addFlags( WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON );

        setContentView( R.layout.tutorial2_surface_view );

        m_open_cv_camera_view = (CameraBridgeViewBase) findViewById( R.id.tutorial2_activity_surface_view );
        m_open_cv_camera_view.setCvCameraViewListener( this );
    }

    @Override
    public void onPause()
    {
        super.onPause();
        if ( m_open_cv_camera_view != null )
            m_open_cv_camera_view.disableView();
    }

    @Override
    public void onResume()
    {
        super.onResume();
        OpenCVLoader.initAsync( OpenCVLoader.OPENCV_VERSION_2_4_3, this, mLoaderCallback );
    }

    public void onDestroy()
    {
        super.onDestroy();
        if ( m_open_cv_camera_view != null )
            m_open_cv_camera_view.disableView();
    }

    public void onCameraViewStarted( int width, int height )
    {
    	m_rgba = new Mat( height, width, CvType.CV_8UC4 );
    }

    public void onCameraViewStopped()
    {
    	m_rgba.release();
    }

    public Mat onCameraFrame( CvCameraViewFrame inputFrame )
    {
    	if ( m_view_mode == VIEW_MODE_DRAW )
    	{
        	m_rgba = inputFrame.rgba();
    		Core.putText( m_rgba, "touch me", new Point( 70, 40 ), Core.FONT_HERSHEY_PLAIN, 2, new Scalar( 255, 0, 0 ), 2 );
    	}
    	else if ( m_view_mode == VIEW_MODE_RECOG )
    	{
            InitRecognizer();
        	m_rgba = inputFrame.rgba();
        	FindFeatures( m_rgba.getNativeObjAddr() );
        	m_view_mode = VIEW_MODE_SAVE;
    		Core.putText( m_rgba, "touch me", new Point( 70, 40 ), Core.FONT_HERSHEY_PLAIN, 2, new Scalar( 255, 0, 0 ), 2 );
    	}
    	else if ( m_view_mode == VIEW_MODE_SAVE )
    	{
    	}
        return m_rgba;
    }

    public boolean onTouchEvent( android.view.MotionEvent event )
    {
    	final double cur_ticks = (double)Core.getTickCount();
    	if ( ( cur_ticks - m_last_ticks ) / Core.getTickFrequency() > 3 )
    	{
	    	if ( m_view_mode == VIEW_MODE_DRAW )
	    	{
	    		m_view_mode = VIEW_MODE_RECOG;
	    	}
	    	else if ( m_view_mode == VIEW_MODE_SAVE )
	    	{
	    		m_view_mode = VIEW_MODE_DRAW;
	    	}
	    	m_last_ticks = cur_ticks;
    	}
    	return true;
    }


    public native void FindFeatures( long mat_addr_rgba );
    public native void InitRecognizer();
}
