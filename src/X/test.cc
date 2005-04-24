/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include "fl/slideshow.h"
#include "fl/gl.h"
#include "fl/pi.h"
#include "fl/matrix.h"

#include <GL/glu.h>

//#include <unistd.h>
#include <iostream>


using namespace std;
using namespace fl;


GLfloat vertices[12] = {0, 0, -1,
						4, 0, -1,
						4, 4, -1,
						0, 4, -1};


class GLTest : public GLShow
{
public:
  void initContext ()
  {
	glClearColor (1, 1, 1, 0);
	glShadeModel (GL_FLAT);
	//glShadeModel (GL_SMOOTH);

	glEnable (GL_DEPTH_TEST);
	//glEnable (GL_NORMALIZE);
	//glEnable (GL_COLOR_MATERIAL);
	glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	glEnable (GL_LIGHTING);
	glEnable (GL_LIGHT0);
  }

  void reshape (int w, int h)
  {
	cerr << "reshape" << endl;

	glViewport (0, 0, w, h);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();

	//glOrtho (-4, 4, -4, 4, -4, 4);
	//glFrustum (0, 4, 0, 4, 2, 100);
	gluPerspective (45, (float) w / (float) h, 3, 100);
  }

void displayCameraIcon (Matrix<double> & camera)
{
  glPushMatrix ();

  // Add transformation to move camera from origin to desired position


  // Draw camera
  float u = 1;  // Unit distance; radius of cone, and 1/2 length of x and y sides of box
  float zb = 2 * u;  // z extent of back end of camera box
  float zc = -1 * u;  // z extend of cone

  //   sides
  float blue[] = {0.5, 0.5, 1, 1};
  glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, blue);
  glBegin (GL_QUADS);
  glNormal3f (1, 0, 0);
  glVertex3f (u, u, 0);
  glVertex3f (u, u, zb);
  glVertex3f (u, -u, zb);
  glVertex3f (u, -u, 0);

  glNormal3f (0, -1, 0);
  glVertex3f (u, -u, 0);
  glVertex3f (u, -u, zb);
  glVertex3f (-u, -u, zb);
  glVertex3f (-u, -u, 0);

  glNormal3f (-1, 0, 0);
  glVertex3f (-u, -u, 0);
  glVertex3f (-u, -u, zb);
  glVertex3f (-u, u, zb);
  glVertex3f (-u, u, 0);

  glNormal3f (0, 1, 0);
  glVertex3f (-u, u, 0);
  glVertex3f (-u, u, zb);
  glVertex3f (u, u, zb);
  glVertex3f (u, u, 0);

  //   front and back
  glNormal3f (0, 0, -1);
  glVertex3f (u, u, 0);
  glVertex3f (u, -u, 0);
  glVertex3f (-u, -u, 0);
  glVertex3f (-u, u, 0);

  glNormal3f (0, 0, 1);
  glVertex3f (u, u, zb);
  glVertex3f (-u, u, zb);
  glVertex3f (-u, -u, zb);
  glVertex3f (u, -u, zb);
  glEnd ();

  //   cone
  float red[] = {1, 0.5, 0.5, 1};
  glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, red);
  glBegin (GL_TRIANGLES);
  const int steps = 10;
  const float angleStep = 2 * PI / steps;
  Vector<float> v0 (3);
  Vector<float> v1 (3);
  Vector<float> v2 (3);
  v0.clear ();
  v1[2] = zc;
  v2[2] = zc;
  Vector<float> n (3);
  for (int i = 0; i < steps; i++)
  {
	float angle1 = i * angleStep;
	float angle2 = (i + 1) * angleStep;
	v1[0] = u * cos (angle1);
	v1[1] = u * sin (angle1);
	v2[0] = u * cos (angle2);
	v2[1] = u * sin (angle2);
	n = v1.cross (v2);
	//n.normalize ();

	glNormal3fv ((float *) n.data);
	glVertex3fv ((float *) v0.data);
	glVertex3fv ((float *) v1.data);
	glVertex3fv ((float *) v2.data);
  }
  glEnd ();


  glPopMatrix ();
}

  void display ()
  {
	cerr << "display" << endl;

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	//gluLookAt (2, 2, 0, 2, 2, -1, 0, 1, 0);

	glTranslatef (0, 0, -20);
	glRotatef (120, 0, 1, 0);
	glRotatef (60, 0, 0, 1);

	/*
	glColor3f (0, 1, 0);
	glBegin(GL_POLYGON);
	glVertex3f(0.0, 0.0, -1);
	glVertex3f(0.0, 0.5, -1);
	glVertex3f(0.5, 0.5, -1);
	glVertex3f(0.75, 0.25, -1);
	glVertex3f(0.5, 0.0, -1);
	glEnd();
	*/

	Matrix<double> A;
	displayCameraIcon (A);

	swapBuffers ();
  }
};


int
main (int argc, char * argv[])
{
  try
  {
	/*
	new ImageFileFormatPGM;

	Image image;
	image.read (argc > 1 ? argv[1] : "bob.pgm");

	cerr << "done reading" << endl;
	SlideShow ss;
	ss.setWMName ("bob");
	ss.show (image);
	ss.waitForClick ();
	*/


	fl::GLXWindow window;
	fl::GLXContext context;

	window.map ();
	window.makeCurrent (context);
	//fl::Colormap colormap (window.screen->defaultVisual (), AllocNone);
	//window.setColormap (colormap);

	//sleep (1);
	glClearColor (1,1,0,1);
	glClear (GL_COLOR_BUFFER_BIT);
	window.swapBuffers ();
	sleep (2);


	/*
	GLTest window;
	window.map ();
	window.screen->display->flush ();
	window.waitForClose ();
	*/
  }
  //catch (const char * error)
  catch (int error)
  {
	cerr << "Exception: " << error << endl;
	return 1;
  }

  return 0;
}
