/**************************
 * Includes
 *
 **************************/

#include <windows.h>
#include "main.h"
#include <gl/gl.h>
#include <gl/glu.h>

/**************************
 * Function Declarations
 *
 **************************/

LRESULT CALLBACK WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void EnableOpenGL (HWND hWnd, HDC *hDC, HGLRC *hRC);
void DisableOpenGL (HWND hWnd, HDC hDC, HGLRC hRC);


const int width=800;
const int height=600;

float f = 0.4;

BOOL FullScreen = FALSE;
BOOL showmenu = TRUE;
WINDOWPLACEMENT wpc;
HMENU menu;

float colors[6][4][3]=
{
0.0, 1.0, 1.0,
0.0, 1.0, 1.0,
0.0, 1.0, 1.0,
0.0, 1.0, 1.0,
1.0, 1.0, 0.0,
1.0, 1.0, 0.0,
1.0, 1.0, 0.0,
1.0, 1.0, 0.0,
1.0, 0.0, 1.0,
1.0, 0.0, 1.0,
1.0, 0.0, 1.0,
1.0, 0.0, 1.0,
0.0, 0.0, 1.0,
0.0, 0.0, 1.0,
0.0, 0.0, 1.0,
0.0, 0.0, 1.0,
0.0, 1.0, 0.0,
0.0, 1.0, 0.0,
0.0, 1.0, 0.0,
0.0, 1.0, 0.0,
1.0, 0.0, 0.0,
1.0, 0.0, 0.0,
1.0, 0.0, 0.0,
1.0, 0.0, 0.0,
};

float cube[6][4][3]=
{
-1.0, -1.0, -1.0,
-1.0, -1.0, 1.0,
-1.0, 1.0, 1.0,
-1.0, 1.0, -1.0,

-1.0, -1.0, -1.0,
-1.0, 1.0, -1.0,
1.0, 1.0, -1.0,
1.0, -1.0, -1.0,

-1.0, -1.0, -1.0,
1.0, -1.0, -1.0,
1.0, -1.0, 1.0,
-1.0, -1.0, 1.0,

-1.0, -1.0, 1.0,
1.0, -1.0, 1.0,
1.0, 1.0, 1.0,
-1.0, 1.0, 1.0,

-1.0, 1.0, -1.0,
-1.0, 1.0, 1.0,
1.0, 1.0, 1.0,
1.0, 1.0, -1.0,

1.0, -1.0, -1.0,
1.0, 1.0, -1.0,
1.0, 1.0, 1.0,
1.0, -1.0, 1.0,
};

void drawAxes()
{
glLineWidth(5);  // толщина линии
glBegin(GL_LINES);
glColor3f(0.6, 0.0, 0.0); glVertex3f(0.0, 0.0, 0.0); glVertex3f(3.0, 0.0, 0.0);
glColor3f(0.0, 0.6, 0.0); glVertex3f(0.0, 0.0, 0.0); glVertex3f(0.0, 3.0, 0.0);
glColor3f(0.0, 0.0, 0.6); glVertex3f(0.0, 0.0, 0.0); glVertex3f(0.0, 0.0, 3.0);
glEnd();
}

void drawCube()
{
int i,j;
for (i = 0; i < 6; i++)
    {
    glBegin(GL_QUADS);
    for (j = 0; j < 4; j++)
        {
        glColor3f(colors[i][j][0], colors[i][j][1], colors[i][j][2]);
        glVertex3f(cube[i][j][0], cube[i][j][1], cube[i][j][2]);
        }
    glEnd();
    }
}



void changeSize(int w, int h) 
{
if (h == 0) h = 1;
//float ratio =  w * 1.0 / h;
float ratio =  (float)w / (float)h;
glMatrixMode(GL_PROJECTION);
glLoadIdentity();
glViewport(0, 0, w, h);
gluPerspective(45.0f, ratio, 0.1f, 100.0f);
glMatrixMode(GL_MODELVIEW);

}


void Init()
{
glClearColor(0,0,0,1);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
glEnable(GL_DEPTH_TEST);

changeSize(width, height);

glTranslatef(0.0, 0.0,-6.0);

glRotatef(30.0, 1.0, 1.0, 1.0); 
}


/**************************
 * WinMain
 *
 **************************/
 
 
 
 
HINSTANCE hInstance;

int WINAPI WinMain (HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine,
                    int iCmdShow)
{
    WNDCLASS wc;
    HWND hWnd;
    HDC hDC;
    HGLRC hRC;        
    MSG msg;
    BOOL bQuit = FALSE;


    /* register window class */
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor (NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject (BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "GLSample";
    RegisterClass (&wc);

    /* create main window */
    hWnd = CreateWindow (
      "GLSample", "OpenGL Sample", 
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      0, 0, width, height,
      NULL, NULL, hInstance, NULL);
	  
	menu = LoadMenu(hInstance, MAKEINTRESOURCE(ID_MENU));
    SetMenu(hWnd, menu); 

    /* enable OpenGL for the window */
    EnableOpenGL (hWnd, &hDC, &hRC);
	
	Init();
	
    /* program main loop */
    while (!bQuit)
    {
        /* check for messages */
        if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
        {
            /* handle or dispatch messages */
            if (msg.message == WM_QUIT)
            {
			bQuit = TRUE;
            }
            else
            {
			TranslateMessage (&msg);
			DispatchMessage (&msg);
            }
        }
        else
        {
		/* OpenGL animation code goes here */
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		drawAxes();
		drawCube();
		
		glRotatef(f, 1.0, 1.0, 1.0); 		
		
		SwapBuffers (hDC);

		Sleep (1);
        }
    }

    /* shutdown OpenGL */
    DisableOpenGL (hWnd, hDC, hRC);

    /* destroy the window explicitly */
    DestroyWindow (hWnd);

    return msg.wParam;
}


/********************
 * Window Procedure
 *
 ********************/

LRESULT CALLBACK WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

switch (message)
    {
	case WM_COMMAND:
	   switch( wParam )
	   {
		 case IDM_FILENEW:
		 case IDM_FILEOPEN:
		 case IDM_FILESAVE:
		 case IDM_FILESAVEAS:
		 case IDM_FILEPRINT:
		 case IDM_FILEPAGESETUP:
		 case IDM_FILEPRINTSETUP:

		 case IDM_EDITUNDO:
		 case IDM_EDITCUT:
		 case IDM_EDITCOPY:
		 case IDM_EDITPASTE:
		 case IDM_EDITDELETE:
			  MessageBox( hWnd, (LPSTR) "Function Not Yet Implemented.",
						  (LPSTR) "GLSample",
						  MB_ICONINFORMATION | MB_OK );
			  return 0;

		 case IDM_HELPCONTENTS:
			  WinHelp( hWnd, (LPSTR) "HELPFILE.HLP",
					   HELP_CONTENTS, 0L );
			  return 0;

		 case IDM_HELPSEARCH:
			  WinHelp( hWnd, (LPSTR) "HELPFILE.HLP",
					   HELP_PARTIALKEY, 0L );
			  return 0;

		 case IDM_HELPHELP:
			  WinHelp( hWnd, (LPSTR) "HELPFILE.HLP",
					   HELP_HELPONHELP, 0L );
			  return 0;

		 case IDM_FILEEXIT:
			  SendMessage( hWnd, WM_CLOSE, 0, 0L );
			  return 0;

		 case IDM_HELPABOUT:
			  MessageBox (NULL, "About..." , "Windows example version 0.01", 1);
			  return 0;

	   }
	return 0;	
	
	
	
    case WM_CREATE:
        return 0;
    case WM_CLOSE:
        PostQuitMessage (0);
        return 0;
	case WM_SIZE:
		{
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);
		changeSize(width, height);
		}
		return 0;
    case WM_DESTROY:
        return 0;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:
            PostQuitMessage(0);
            return 0;
        case VK_UP:
            f+=0.1;
            return 0;	
        case VK_F1:
			{
            if (showmenu) {SetMenu(hWnd, NULL); showmenu = FALSE;}
			else {SetMenu(hWnd, menu); showmenu = TRUE;}
			}
            return 0;			
        case VK_DOWN:
            f-=0.1;
            return 0;		
		case VK_SPACE:
			{
			if(!FullScreen)//Из оконного во весь экран
				{
				GetWindowPlacement(hWnd,&wpc);//Сохраняем параметры оконного режима
				SetWindowLong(hWnd, GWL_STYLE, WS_POPUP);//Устанавливаем новые стили
				SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);
				ShowWindow(hWnd, SW_SHOWMAXIMIZED);//Окно во весь экран
				SetMenu(hWnd, NULL); showmenu = FALSE;
				FullScreen = TRUE;
				}
			else//Из всего эранна в оконное
				{     
				SetWindowLong(hWnd, GWL_STYLE,WS_OVERLAPPEDWINDOW | WS_VISIBLE);//Устанавливаем стили окнного режима
				SetWindowLong(hWnd, GWL_EXSTYLE,0L);
				SetWindowPlacement(hWnd,&wpc);//Загружаем парметры предыдущего оконного режима
				ShowWindow(hWnd, SW_SHOWDEFAULT);//Показываем обычное окно
				SetMenu(hWnd, menu); showmenu = TRUE;
				FullScreen = FALSE;
				}
			}
		return 0; 
        }
        return 0;

    default:
        return DefWindowProc (hWnd, message, wParam, lParam);
    }
}


/*******************
 * Enable OpenGL
 *
 *******************/

void EnableOpenGL (HWND hWnd, HDC *hDC, HGLRC *hRC)
{
    PIXELFORMATDESCRIPTOR pfd;
    int iFormat;

    /* get the device context (DC) */
    *hDC = GetDC (hWnd);

    /* set the pixel format for the DC */
    ZeroMemory (&pfd, sizeof (pfd));
    pfd.nSize = sizeof (pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;
    iFormat = ChoosePixelFormat (*hDC, &pfd);
    SetPixelFormat (*hDC, iFormat, &pfd);

    /* create and enable the render context (RC) */
    *hRC = wglCreateContext( *hDC );
    wglMakeCurrent( *hDC, *hRC );

}


/******************
 * Disable OpenGL
 *
 ******************/

void DisableOpenGL (HWND hWnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent (NULL, NULL);
    wglDeleteContext (hRC);
    ReleaseDC (hWnd, hDC);
}
