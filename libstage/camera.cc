/*
 *  camera.cc
 *  Stage
 *
 *  Created by Alex Couture-Beil on 06/06/08.
 *
 */


#include "stage_internal.hh"

#include <iostream>

// Perspective Camera
StgPerspectiveCamera::StgPerspectiveCamera( void ) : 
	StgCamera(),
	_z_near( 0.2 ), _z_far( 40.0 ),
	_vert_fov( 70 ), _horiz_fov( 70 ),
	_aspect( 1.0 )
{
	setPitch( 70.0 );
}

void StgPerspectiveCamera::move( float x, float y, float z )
{
	//scale relative to zoom level
	x *= _z / 100.0;
	y *= _z / 100.0;
	
	//adjust for yaw angle
	_x += cos( dtor( _yaw ) ) * x;
	_x += -sin( dtor( _yaw ) ) * y;
	
	_y += sin( dtor( _yaw ) ) * x;
	_y += cos( dtor( _yaw ) ) * y;
	}

void StgPerspectiveCamera::Draw( void ) const
{	
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	
	glRotatef( - _pitch, 1.0, 0.0, 0.0 );
	glRotatef( - _yaw, 0.0, 0.0, 1.0 );

	glTranslatef( - _x, - _y, - _z );
	//zooming needs to happen in the Projection code (don't use glScale for zoom)

}

void StgPerspectiveCamera::SetProjection( void ) const
{
//	SetProjection( pixels_width/pixels_height );
	
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	
	float top = tan( dtor( _vert_fov ) / 2.0 ) * _z_near;
	float bottom = -top;
	float right = tan( dtor( _horiz_fov ) / 2.0 ) * _z_near;
	float left = -right;
	
	right *= _aspect;
	left *= _aspect;

	glFrustum( left, right, bottom, top, _z_near, _z_far );
	
	glMatrixMode (GL_MODELVIEW);
	
}

void StgPerspectiveCamera::update( void )
{	
}


void StgPerspectiveCamera::strafe( float amount )
{
	_x += cos( dtor( _yaw ) ) * amount;
	_y += sin( dtor( _yaw ) ) * amount;
}

void StgPerspectiveCamera::forward( float amount )
{
	_x += -sin( dtor( _yaw ) ) * amount;
	_y += cos( dtor( _yaw ) ) * amount;
}

void StgPerspectiveCamera::Load( Worldfile* wf, int sec ) {
	float x = wf->ReadTupleFloat(sec, "pcam_loc", 0, 0 );
	float y = wf->ReadTupleFloat(sec, "pcam_loc", 1, 0 );
	float z = wf->ReadTupleFloat(sec, "pcam_loc", 2, 0 );
	setPose( x, y, z );
	setPitch( wf->ReadTupleFloat( sec, "pcam_angle", 0, 0 ) );
	setYaw( wf->ReadTupleFloat( sec, "pcam_angle", 1, 0 ) );
}

void StgPerspectiveCamera::Save( Worldfile* wf, int sec ) {
	wf->WriteTupleFloat( sec, "pcam_loc", 0, x() );
	wf->WriteTupleFloat( sec, "pcam_loc", 1, y() );
	wf->WriteTupleFloat( sec, "pcam_loc", 2, z() );
	wf->WriteTupleFloat( sec, "pcam_angle", 0, pitch()  );
	wf->WriteTupleFloat( sec, "pcam_angle", 1, yaw()  );
}




//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Ortho camera
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void StgOrthoCamera::Draw( void ) const
{	
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	glRotatef( _pitch, 1.0, 0.0, 0.0 );
	glRotatef( _yaw, 0.0, 0.0, 1.0 );

	glTranslatef( - _x, - _y, 0.0 );
	//zooming needs to happen in the Projection code (don't use glScale for zoom)

}

void StgOrthoCamera::SetProjection( void ) const
{
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	
	glOrtho( -_pixels_width/2.0 / _scale, _pixels_width/2.0 / _scale,
			-_pixels_height/2.0 / _scale, _pixels_height/2.0 / _scale,
			_y_min * _scale * 2, _y_max * _scale * 2 );	
	
	glMatrixMode (GL_MODELVIEW);
}

void StgOrthoCamera::SetProjection( float pixels_width, float pixels_height, float y_min, float y_max )
{
	_pixels_width = pixels_width;
	_pixels_height = pixels_height;
	_y_min = y_min; 
	_y_max = y_max;
	SetProjection();
}

void StgOrthoCamera::move( float x, float y ) {
	//convert screen points into world points
	x = x / ( _scale );
	y = y / ( _scale );
	
	//adjust for pitch angle
	y = y / cos( dtor( _pitch ) );
	
	//don't allow huge values
	if( y > 100 ) 
		y = 100;
	else if( y < -100 ) 
		y = -100;
	
	//adjust for yaw angle
	_x += cos( dtor( _yaw ) ) * x;
	_y += -sin( dtor( _yaw ) ) * x;
	
	_x += sin( dtor( _yaw ) ) * y;
	_y += cos( dtor( _yaw ) ) * y;
}

//TODO re-evaluate the way the camera is shifted when the mouse zooms - it might be possible to simplify
void StgOrthoCamera::scale( float scale, float shift_x, float w, float shift_y, float h )
{
	float to_scale = -scale;
	const float old_scale = _scale;

	//TODO setting up the factor can use some work
	float factor = 1.0 + fabs( to_scale ) / 25;
	if( factor < 1.1 )
		factor = 1.1; //this must be greater than 1.
	else if( factor > 2.5 )
		factor = 2.5;

	//convert the shift distance to the range [-0.5, 0.5]
	shift_x = shift_x / w - 0.5;
	shift_y = shift_y / h - 0.5;

	//adjust the shift values based on the factor (this represents how much the positions grows/shrinks)
	shift_x *= factor - 1.0;
	shift_y *= factor - 1.0;

	if( to_scale > 0 ) {
		//zoom in
		_scale *= factor;
		move( shift_x * w / _scale * _scale, 
				- shift_y * h / _scale * _scale );
	}
	else {
		//zoom out
		_scale /= factor;
		if( _scale < 1 ) {
			_scale = 1;
		} else {
			//shift camera to follow where mouse zoomed out
			move( - shift_x * w / old_scale * _scale, 
					shift_y * h / old_scale * _scale );
		}
	}
}

void StgOrthoCamera::Load( Worldfile* wf, int sec ) {
	float x = wf->ReadTupleFloat(sec, "center", 0, 0 );
	float y = wf->ReadTupleFloat(sec, "center", 1, 0 );
	setPose( x, y );
	setPitch( wf->ReadTupleFloat( sec, "rotate", 0, 0 ) );
	setYaw( wf->ReadTupleFloat( sec, "rotate", 1, 0 ) );
	setScale( wf->ReadFloat(sec, "scale", scale() ) );
}

void StgOrthoCamera::Save( Worldfile* wf, int sec ) {
	wf->WriteTupleFloat( sec, "center", 0, x() );
	wf->WriteTupleFloat( sec, "center", 1, y() );
	wf->WriteTupleFloat( sec, "rotate", 0, pitch() );
	wf->WriteTupleFloat( sec, "rotate", 1, yaw() );
	wf->WriteFloat(sec, "scale", scale() );
}

