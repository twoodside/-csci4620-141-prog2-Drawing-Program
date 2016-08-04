//Tyler Woodside
//CSCI 4620 
//Stanley Wileman

#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include <stdio.h>
#include "LineHeader.h"
#include "menu.h"

LineList *listHead;//Linked list containing all lines
Line *drawLine;//Used for user feedback
char* fname;//File name
int various;//Various booleans saved via bitwise operations. Full list below.
            //any line selected(9),current tool(2 bits,8&7),free,moving endpoint(6),zoom factor(2 bits, 5&4),help sidebar(3),big endpoints(2),save or load(1),cursor flash(0)
int prevX,prevY;//The previous x and y values when moving a line
int tranX,tranY;//The amount to shift the drawing. Used for zooming.

//See functions for explanation
//Setup functions
void init(int argc,char **argv);
void clearDrawLine();
void save();
void load();
void drawHovertext(char* str,int n,int x,int y);
void zoom(int zoomout,int x,int y);
void printHelp();

//Callbacks
void disp(void);
void resize(int width,int height);
void mouseControl(int button, int state,int x, int y);
void moveControl(int x,int y);
void passiveControl(int x,int y);
void keyControl(unsigned char key,int x,int y);

//Textbox to specify files for saving/loading
void textboxInit(int varInit);
void textboxDisp(void);
void textboxKeyControl(unsigned char key,int x,int y);
void textboxClose(int saveload);
void textboxPipeTimer(int value);
/**
init(argc,argv);
Initializes the main window, file processing, and globals.
int argc - Number of command line arguments
char **argv - Command line arguments. Here, used to specify the name of the file to start with.
**/
void init(int argc,char **argv){
    int width=400;
    int height=400;
    FILE *in;
    
    //Initialize globals
    listHead=NULL;
    clearDrawLine();
    various=0;
    prevX=prevY=-1;
    
    //Set up window
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(400,400);
    glutInitWindowPosition( ( glutGet(GLUT_SCREEN_WIDTH)-400 )/2,( glutGet(GLUT_SCREEN_HEIGHT)-400 )/2);
    if (argc>1){
        glutCreateWindow(argv[1]);
        glutSetIconTitle(argv[1]);
    }
    else{
        glutCreateWindow("Untitled");
        glutSetIconTitle("Untitled");
    }
    
    
    glutMouseFunc(mouseControl);
    glutMotionFunc(moveControl);
    glutPassiveMotionFunc(passiveControl);
    glutKeyboardFunc(keyControl);
    glutDisplayFunc(disp);
    glutReshapeFunc(resize);
    
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0.0, width, height, 0.0);
    if (argc>1){
        fname=argv[1];
        load();
    }
    else{
        fname="";
        fprintf(stderr,"No file specified.\n");
    }
    
    createDisplayList();
    
    glLineStipple(1,0xFF);
    
    glutPostRedisplay();
}

/**
clearDrawLine();
Resets the drawing line so it's not visible and not attached to any other lines.
**/
void clearDrawLine(){
    Line *newLine;
    newLine=malloc(sizeof(Line));
    
    newLine->left[0]=-33;
    newLine->left[1]=-33;
    newLine->right[0]=-33;
    newLine->right[1]=-33;
    
    if (drawLine==NULL){
        newLine->thickness=1;
    }
    else{
        newLine->thickness=drawLine->thickness;
    }
    newLine->selected=0;
    
    drawLine=newLine;
}

/**
save();
Saves to fname. If fname is an empty string, will create a textbox prompting the user for a file name to save to.
**/
void save(){
    FILE* out;
    LineList *i=listHead;
    if ( fname[0]!='\0' ){
        out=fopen(fname,"w");
        
        if ( out!=NULL ){
            fprintf( out,"%d %d\n",glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT) );
            
            while (i!=NULL){
                fprintf(out,"%d %d %d %d %d\n",i->line->left[0],i->line->left[1],i->line->right[0],i->line->right[1],i->line->thickness);
                i=i->next;
            }
        }
        else{
            fprintf(stderr,"Could not open \"%s\" for write.\n",fname);
        }
        fclose(out);
    }
    else{
        textboxInit(0);
    }
}

/**
load();
Opens a file from fname. Will NOT prompt user for file name - this should only be called via the textbox itself. To load, call textboxInit(1);
**/
void load(){
    FILE *in;
    int width,height;
    
    listHead=NULL;
    clearDrawLine();
    various &= ~(3); //Set right-most 2 bits to 0
    
    in=fopen(fname,"r");
    if ( in!=NULL && !feof(in) ){
        fscanf(in,"%d %d",&width,&height);
        glutReshapeWindow(width,height);    
        while (!feof(in)){
            int v1[2],v2[2];
            unsigned int thick;
            unsigned char sel;
            fscanf(in,"%d %d %d %d %d",&v1[0],&v1[1],&v2[0],&v2[1],&thick);
            if (v1[0] && v1[1] && v2[0] && v2[1] && thick){
                listHead=addCoords(v1,v2,thick,listHead);
            }
        }
    }
    else{
        fprintf(stderr,"No file found.\n");
    }
    
    fclose(in);
}
/**
glutDisplayFunc(disp);
Callback function for display. Draws the line list stored in listHead, then the drawLine, then the toolbars, hovertext, and zoom guide.
**/
void disp(void){
    LineList* i=listHead;
    int n;
    int scale;
    int *m;
    char *p;
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    //LineList
    while (i!=NULL){
        if (i->line->selected==1){
            glEnable(GL_LINE_STIPPLE);
        }
        glLineWidth(i->line->thickness);
        glBegin(GL_LINES);
            glVertex2iv(i->line->left);
            glVertex2iv(i->line->right);
        glEnd();
        if (i->line->selected==1){
            glDisable(GL_LINE_STIPPLE);
        }
        if ( various&(1<<2) ){
            n=(3+i->line->thickness)/2;
            glRecti(i->line->left[0]-n,i->line->left[1]-n,i->line->left[0]+n,i->line->left[1]+n);
            glRecti(i->line->right[0]-n,i->line->right[1]-n,i->line->right[0]+n,i->line->right[1]+n);
        }
        i=i->next;
    }
    
    //DrawLine
    
    //glColor3f(1.0,0,0);
    
    if (drawLine->selected){
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1,0xAA00);
    }
    
    glLineWidth( drawLine->thickness );
    glBegin(GL_LINES);
        glVertex2iv(drawLine->left);
        glVertex2iv(drawLine->right);
    glEnd();
    if ( various&(1<<2) ){
        n=( drawLine->thickness+3 )/2;
        glRecti(drawLine->left[0]-n,drawLine->left[1]-n,drawLine->left[0]+n,drawLine->left[1]+n);
        glRecti(drawLine->right[0]-n,drawLine->right[1]-n,drawLine->right[0]+n,drawLine->right[1]+n);
    }
    
    if (drawLine->selected){
        glDisable(GL_LINE_STIPPLE);
        glLineStipple(1,0xFF);
    }
    glLineWidth(1);
    //glColor3i(0,0,0);
    
    
    //Toolbar
    scale=1<<( (various>>4)&3 );
    glPushMatrix();
    glTranslatef(-tranX,-tranY,0);
    glScalef(1.0/scale,1.0/scale,1);
    if ( !(various&(1<<3)) ){
        glCallList(1);
        glPushMatrix();
            glTranslatef(39,10,0);
            glScalef(0.07,-0.07,1);
            switch ( (various>>8)&11){
                case 0:
                    p="Pen";
                    break;
                case 1:
                    p="Select";
                    break;
                case 2:
                    p="Zoom in";
                    break;
                case 3:
                    p="Zoom out";
                    break;
            }
            
            n=0;
            for (;*p;p++){
                glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
                n+=glutStrokeWidth(GLUT_STROKE_ROMAN, *p);
            }
            p=malloc(sizeof(char)*3);
            sprintf(p,"Width:%d",drawLine->thickness);
            glTranslatef(-n,-142.85714285714285714285714285714,0);
            for (;*p;p++){
                glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
            }
        glPopMatrix();
    }
    
    glCallList(2);
    
    glPopMatrix();
    
    
    if ( (various&(1<<9)) && !(various&(1<<8)) ){
        scale=1<<((various>>4)&3);
        n=glutGet(GLUT_WINDOW_WIDTH)/(scale*4);
        scale=glutGet(GLUT_WINDOW_HEIGHT)/(scale*4);
        
        glColor3f(1.0,0,0);
        glBegin(GL_LINE_LOOP);
            glVertex2i(prevX-n,prevY-scale);
            glVertex2i(prevX+n,prevY-scale);
            glVertex2i(prevX+n,prevY+scale);
            glVertex2i(prevX-n,prevY+scale);
        glEnd();
        glColor3i(0,0,0);
    }
    
    glFlush();
}

/**
glutReshapeFunc(resize);
Callback function for reshaping of the main window. Resets the zoom, resizes the viewing area, then centers the window.
int width - width to resize to.
int height - height to resize to.
**/
void resize(int width,int height){
    if (height<128){
        glutReshapeWindow(width,128);
        return;
    }
    if (width<86){
        glutReshapeWindow(86,height);
        return;
    }
    
    zoom(-1,0,0);
    
    glLoadIdentity();
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glColor3i(0,0,0);
    
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0.0, width, height, 0.0);
    glViewport(0, 0, width, height);
    glutPositionWindow(( glutGet(GLUT_SCREEN_WIDTH)-width )/2,( glutGet(GLUT_SCREEN_HEIGHT)-height )/2);
}

/**
zoom(1,x,y);
Zooms in or out depending on the value of zoomout. Centers the zoom on the given x and y, without going into negative area.
int zoomout - 1 for zoom out, 0 for zoom in.
int x - x center of zoom area. Mouse x.
int y - y center of zoom area. Mouse y.
**/
void zoom(int zoomout, int x,int y){
    int width,height;
    if (zoomout==1){
        if ( ((various>>4)&3)<4 && ((various>>4)&3)>0){
            glTranslatef(-tranX,-tranY,0);
            tranX=tranY=0;
            glScalef(.5,.5,1);
            various-=(1<<4);
            
            zoomout=1<<((various>>4)&3);
            width=glutGet(GLUT_WINDOW_WIDTH)/(zoomout*2);
            height=glutGet(GLUT_WINDOW_HEIGHT)/(zoomout*2);
            
            tranX=width-x;
            tranY=height-y;
            if (tranX>0){
                tranX=0;
            }
            if (tranY>0){
                tranY=0;
            }
            
            glTranslatef(tranX,tranY,0);
        }
    }
    else if (zoomout==0){
        if ( ((various>>4)&3)<3){
            glTranslatef(-tranX,-tranY,0);
            tranX=tranY=0;
            glScalef(2,2,1);
            various+=(1<<4);
            
            zoomout=1<<((various>>4)&3);
            width=glutGet(GLUT_WINDOW_WIDTH)/(zoomout*2);
            height=glutGet(GLUT_WINDOW_HEIGHT)/(zoomout*2);
            
            tranX=width-x;
            tranY=height-y;
            if (tranX>0){
                tranX=0;
            }
            if (tranY>0){
                tranY=0;
            }
            
            glTranslatef(tranX,tranY,0);
        }
    }
    else{
        glTranslatef(-tranX,-tranY,0);
        zoomout=1<<((various>>4)&3);
        glScalef(1.0/zoomout,1.0/zoomout,1);
        tranX=tranY=0;
        various&=~(11<<4);
    }
    
    glutPostRedisplay();
}

/**
drawHovertext("Hello World!",0,x,y);
Creates a display list in list 2 for a hovertext containing str, assuming x and y are the mouse x and y.
char* str - String to draw.
int n - Not used, too many calls to remove it from.
int x - Mouse x.
int y - Mouse y.
**/
void drawHovertext(char* str,int n,int x,int y){
    char *p;
    int strwidth;
    int *rasterpos1,*rasterpos2;
    y+=20;
    
    strwidth=0;
    for (p = str; *p; p++){
        strwidth+=glutStrokeWidth(GLUT_STROKE_ROMAN,*p);
    }
    strwidth/=10;
    strwidth+=5;
    
    glNewList(2,GL_COMPILE);
        glColor3f(1,.93,.8);
        glRecti(x,y,x+strwidth,y+15);
            
        glColor3i(0,0,0);
        glBegin(GL_LINE_LOOP);
            glVertex2f(x,y);
            glVertex2f(x,y+15.238);
            glVertex2f(x+strwidth,y+15.238);
            glVertex2f(x+strwidth,y);
        glEnd();
            
        glPushMatrix();
            glTranslatef(x+1,y+13,0);
            glScalef(0.1,-0.1,1);
            for (p = str; *p; p++){
                glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
            }
        glPopMatrix();
    glEndList();
}

/**
textboxInti(1);
Creates a textbox for the user to specify a file name.
int varInit - 0 for save, 1 for load. Puts this value into the second bit in various for use later.
**/
void textboxInit(int varInit){
    various |= (varInit<<1); //Set 2nd bit to varInit (1 for load, 0 for save)
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(256,58);
    glutInitWindowPosition( ( glutGet(GLUT_SCREEN_WIDTH)-256 )/2,( glutGet(GLUT_SCREEN_HEIGHT)-58 )/2);
    glutCreateWindow("");
    
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glColor3i(0,0,0);
    
    glutDisplayFunc(textboxDisp);
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0.0, 256.0, 58.0, 0.0);
    glutKeyboardFunc(textboxKeyControl);
    glutTimerFunc(1000,textboxPipeTimer,glutGetWindow() );
    
    glutPostRedisplay();
}

/**
glutDisplayFunc(textboxDisp);
Callback function for the textbox. Draws some boxes and prints instructions + what the user has already entered.
**/
void textboxDisp(void){
    char *p,*saveloadText;
    glClear(GL_COLOR_BUFFER_BIT);
    
    
    saveloadText="Entr file name: (Enter to go)";
    if (various & (1 << 1)){
        strncpy(saveloadText,"Load",4);
    }
    else{
        strncpy(saveloadText,"Save",4);
    }
    
    glBegin(GL_LINE_LOOP);
        glVertex2i(10,22);
        glVertex2i(10,46);
        glVertex2i(246,46);
        glVertex2i(246,22);
    glEnd();
    
    glPushMatrix();
        glTranslatef(11,42,0);
        glScalef(0.1,-0.1,1);
        for (p = fname; *p; p++){
            glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
        }
        if (various & 1){
            glutStrokeCharacter(GLUT_STROKE_ROMAN,'|');
        }
    glPopMatrix();
    
    glPushMatrix();
        glTranslatef(2,15,0);
        glScalef(0.125,-0.125,1);
        for (p = saveloadText; *p; p++){
            glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
        }
    glPopMatrix();
        
    glFlush();
}

/**
textboxPipeTimer(2);
Timer that flashes the pipe on the textbox to indicate that the user can type.
**/
void textboxPipeTimer(int window){
    if (glutGetWindow()==window){
        various ^= 1;
        
        glutTimerFunc(1000,textboxPipeTimer,glutGetWindow());
        glutPostRedisplay();
    }
}

/**
textboxClose(1);
Close the textbox by either saving, loading, or exiting without either.
int saveload - boolean, 1 to save or load depending on the value of the second bit in various, 0 to exit without saving.
**/
void textboxClose(int saveload){
    glutDestroyWindow(2);
    glutSetWindow(1);
    
    if (saveload){
        if ( !(various & (1<<1)) ){
            save();
            glutSetWindowTitle(fname);
            glutSetIconTitle(fname);
        }
        else{
            load();
            glutSetWindowTitle(fname);
            glutSetIconTitle(fname);
        }
    }
    various &= ~(3);
}


/**
glutKeyboardFunc(textboxKeyControl);
Callback for key control for the textbox. Reads in what the user types, processes it into fname, and updates the screen.
unsigned char key - 'a','A',etc.
int x - Mouse x. Not used.
int y - Mouse y. Not used.
**/
void textboxKeyControl(unsigned char key,int x,int y){
    int i=0;
    if (key==13){ //Enter
        textboxClose(1);
    }
    else if (key==8){ //Backspace
        while (fname[i]!='\0'){
            i++;
        }
        fname[--i]='\0';
    }
    else if (key==27){ //Escape
        textboxClose(0);
    }
    else if (key==127){ //Delete
        //Do Nothing 
    }
    else{
        strncat(fname,&key,1);
    }
    
    glutPostRedisplay();
}

/**
glutMotionFunc(moveControl);
Callback for mouse motion when the mouse button is clicked. Moves the draw line around, either by rubber banding if using the pen tool or endpoint selection, or else by moving the whole line proportionally to the last move.
int x - mouse x.
int y - mouse y.
**/
void moveControl(int x,int y){
    int n;
    
    n=1<<( (various>>4)&3 );
    x/=n;
    y/=n;
    x-=tranX;
    y-=tranY;
    
    if (!(various&(3<<8)) && drawLine->left[0]>=0){ //If using the pen tool
        drawLine->right[0]=x;
        drawLine->right[1]=y;
        glutPostRedisplay();
    }
    else if ( (various&(1<<8)) && !(various&(1<<9)) && drawLine->left[0]>=0 && !(various&(1<<6)) ){ //If using the selection tool and not right click
        drawLine->left[0]+=(x-prevX);
        drawLine->right[0]+=(x-prevX);
        drawLine->left[1]+=(y-prevY);
        drawLine->right[1]+=(y-prevY);
        prevX=x;
        prevY=y;
        glutPostRedisplay();
    }
    else if ( (various&(1<<8)) && !(various&(1<<9)) && drawLine->left[0]>=0 && (various&(1<<6)) ){
        drawLine->right[0]=x;
        drawLine->right[1]=y;
        glutPostRedisplay();
    }
}

/**
glutPassiveMotionFunc(passiveControl);
Callback for the mouse motion when the mouse button is not clicked. Draws hovertext if hovered over the menu.
**/
void passiveControl(int x,int y){
    int i,n=1;
    n=1<<( (various>>4)&3 );
    
    if ( (various&(1<<9)) && !(various&(1<<8)) ){
        x/=n;
        y/=n;
        x-=tranX;
        y-=tranY;
        prevX=x;
        prevY=y;
        glutPostRedisplay();
    }
    
    if ((various&(1<<3)) || x>40 || y>128){
        glDeleteLists(2,1);
        glutPostRedisplay();
        return;
    }
    switch ((x-2)/18*10 + (y-2)/18){
        case 0://Pen tool
            drawHovertext("Pen tool (X)",n,x,y);
        break;
        
        case 10://Select tool
            drawHovertext("Select tool (S)",n,x,y);
        break;
        
        case 1://Zoom in
            drawHovertext("Zoom in tool (U)",n,x,y);
        break;
        
        case 11://Zoom out
            drawHovertext("Zoom out tool (D)",n,x,y);
        break;
        
        case 2://Delete
            drawHovertext("Delete (Del)",n,x,y);
        break;
        
        case 12://Reset zoom
            drawHovertext("Reset zoom (R)",n,x,y);
        break;
        
        case 3://Thicken (T)
            drawHovertext("Thicken lines (Shift+T)",n,x,y);
        break;
        
        case 13://Thin
            drawHovertext("Thin lines (T)",n,x,y);
        break;
        
        case 4: //Toggle toolbar
            drawHovertext("Print help to console (H)",n,x,y);
        break;
        
        case 14://Toggle endpoints
            drawHovertext("Toggle endpoints (P)",n,x,y);
        break;

        case 5://Save as
            drawHovertext("Save as (Shift+Ctrl+S)",n,x,y);
        break;
        
        case 15://Toggle toolbar
            drawHovertext("New (Ctrl+N)",n,x,y);
        break;

        case 6://Save
            drawHovertext("Save (Ctrl+S)",n,x,y);
        break;
        
        case 16://Load
            drawHovertext("Load (Ctrl+O)",n,x,y);
        break;
    }
    glutPostRedisplay();
}


/**
glutMouseFunc(mouseControl);
Callback for mouse clicks. Processes appropriately according to the tool.
    Zoom in - Zooms in up to 3 times/1600%.
    Zoom out - Zooms out up to 100%.
    Pen - Draws a line on left click + drag. Press starts the line, release finalizes it if the length>5.
    Select + left click - selects and moves a line. First click will select the closes line by proximity. Shift+subsequent clicks will select more lines. Press, drag, and release will move all selected lines.
    Select + right click - selects and moves an endpoint. Key press selects the endpoint, drag moves it, release finalizes the line. Only one endpoint can be selected at a time.
int button - GLUT_LEFT_BUTTON, GLUT_MIDDLE_BUTTON, or GLUT_RIGHT_BUTTON depending on which the user pressed.
int state - GLUT_DOWN on click, GLUT_UP on release.
int x - Mouse x.
int y - Mouse y.
**/
void mouseControl(int button, int state,int x, int y){
    LineList *i;
    int j,n=1;
    n=1<<( (various>>4)&3 );
    
    //If click on the toolbar
    if (!(various&(1<<3)) && x<40 && y<128 && button==GLUT_LEFT_BUTTON){
        //Unique keycodes for each column + row of buttons
        //Will only activate on key release. Done this way for visual organization's sake.
        //*                  0|100   +      0|10   + [0-7]
        switch ((state==GLUT_DOWN)*100 + (x-2)/18*10 + (y-2)/18){
            case 0://Pen tool
                various&=~(1<<9);
                various&=~(1<<8);
                prevX=prevY=-1;
                deselectLines(listHead);
                glutPostRedisplay();
            break;
            
            case 10: //Select Tool
                various&=~(1<<9);
                various|=(1<<8);
                prevX=prevY=-1;
            break;
            
            case 1: //Zoom in
                various|=(1<<9);
                various&=~(1<<8);
                prevX=prevY=-1;
                deselectLines(listHead);
                glutPostRedisplay();
            break;
            
            case 11: //Zoom out
                various|=(1<<9);
                various|=(1<<8);
                prevX=prevY=-1;
                deselectLines(listHead);
                glutPostRedisplay();
            break;
            
            case 2://Delete line
                listHead=deleteSelectedLines(listHead);
                glutPostRedisplay();
            break;
            
            case 12://Reset Zoom level
                zoom(-1,x,y);
            break;
            
            case 3://Thicken Lines
                if (drawLine->thickness<=0xF){
                    drawLine->thickness++;
                }
            break;
            
            case 13://Thin Lines
                if (drawLine->thickness>1){
                    drawLine->thickness--;
                }
            break;
            
            case 4://Help toggle
                //various^=1<<3;
                //glutPostRedisplay();
                printHelp();
            break;
            
            case 14://Toggle endpoints
                various^=1<<2;
                glutPostRedisplay();
            break;
            
            case 5://Save as
                fname[0]='\0';
                save();
            break;
            
            case 15://New
                listHead=NULL;
                fname="";
                fname[0]='\0';
            break;
            
            case 6://Save
                save();
            break;
            
            case 16://Load
                fname="";
                fname[0]='\0';
                textboxInit(1);
            break;
        }
        return;
    }
    //If using the zoom in tool
    else if ( (various&(1<<9)) && !(various&(1<<8)) ){
        if (button==GLUT_LEFT_BUTTON && state==GLUT_UP){
            zoom(0,x,y);
        }
        return;
    }
    //If using the zoom out tool
    else if ( (various&(1<<9)) && (various&(1<<8)) ){
        if (button==GLUT_LEFT_BUTTON && state==GLUT_UP){
            zoom(1,x,y);
        }
        return;
    }
    else{
        x/=n;
        y/=n;
        x-=tranX;
        y-=tranY;
    }
    
    //If using the pen tool
    if ( !(various&(3<<8)) && button==GLUT_LEFT_BUTTON ){
        
        if (various&(1<<15)){
            x/=2;
            y/=2;
        }
        
        //On button press, start the line
        if (state==GLUT_DOWN){
            drawLine->left[0]=x;
            drawLine->left[1]=y;
            drawLine->right[0]=x;
            drawLine->right[1]=y;
            glutPostRedisplay();
        }
        //On release, finish the line
        else if (drawLine->left[0]>=0){ //Don't do anything if the line wasn't started (by clicking and switching tools, moving out of the toolbar, etc.)
            drawLine->right[0]=x;
            drawLine->right[1]=y;
            
            //Make sure the line is at least 5 pixels long
            if ( 25<(drawLine->right[0]-drawLine->left[0])*(drawLine->right[0]-drawLine->left[0])+(drawLine->right[1]-drawLine->left[1])*(drawLine->right[1]-drawLine->left[1]) ){
                listHead=addLine(drawLine,listHead);
            }
            clearDrawLine();
            glutPostRedisplay();
        }
    }
    //If using the selection tool and left click
    else if ( (various&(1<<8)) && !(various&(1<<9)) && button==GLUT_LEFT_BUTTON ){
        i=listHead;
        while (i!=NULL && !i->line->selected){
            i=i->next;
        }
        //If we aren't moving a line
        if ( state==GLUT_UP && ( drawLine->left[0]<0 || abs(drawLine->left[0] - i->line->left[0])<5 && abs(drawLine->left[1] - i->line->left[1])<5 || i==NULL || glutGetModifiers() & GLUT_ACTIVE_SHIFT) ){
            clearDrawLine();
            selectLine(listHead,x,y,glutGetModifiers() & GLUT_ACTIVE_SHIFT);
            glutPostRedisplay();
        }
        else if ( state==GLUT_DOWN && i!=NULL && !(glutGetModifiers() & GLUT_ACTIVE_SHIFT) ){
            drawLine->left[0]=i->line->left[0];
            drawLine->left[1]=i->line->left[1];
            drawLine->right[0]=i->line->right[0];
            drawLine->right[1]=i->line->right[1];
            drawLine->thickness=i->line->thickness;
            drawLine->selected=1;
            prevX=x;
            prevY=y;
        }
        else if (state==GLUT_UP && i!=NULL){
            x=drawLine->left[0] - i->line->left[0];
            y=drawLine->left[1] - i->line->left[1];
            clearDrawLine();
            
            prevX=prevY=-1;
            
            moveSelectedLines(listHead,x,y);
            
            glutPostRedisplay();
        }
    }
    //Selection tool and right click
    else if ( (various&(1<<8)) && !(various&(1<<9)) && button==GLUT_RIGHT_BUTTON ){
        deselectLines(listHead);
        if (state==GLUT_DOWN){
            Line *sel;
            sel=selectEndpoint(listHead,x,y);
            
            if (sel!=NULL){
                if (sel->selected==1){
                    drawLine->left[0]=sel->right[0];
                    drawLine->left[1]=sel->right[1];
                    drawLine->right[0]=x;
                    drawLine->right[1]=y;
                    drawLine->thickness=sel->thickness;
                    drawLine->selected=1;
                }
                else{
                    drawLine->left[0]=sel->left[0];
                    drawLine->left[1]=sel->left[1];
                    drawLine->right[0]=x;
                    drawLine->right[1]=y;
                    drawLine->thickness=sel->thickness;
                    drawLine->selected=1;
                }
                sel->selected=1;
                listHead=deleteSelectedLines(listHead);
                various|=1<<6;
            }
        }
        else if (drawLine->left[0]>=0){
            drawLine->right[0]=x;
            drawLine->right[1]=y;
            drawLine->selected=0;
            listHead=addLine(drawLine,listHead);
            various&=~(1<<6);
            clearDrawLine();
        }
    }
}


/**
glutKeyboardFunc(keyControl);
Callback function for keyboard press.
     	   H - Redisplay this help
     	   P - Toggle display of big line endpoints
       Q/Esc - Terminate the program
      ctrl+S - Save the drawing
Shift+ctrl+S - Save the drawing to a new name
      Ctrl+O - Open a drawing
      Ctrl+N - Clear drawing
     	   t - Decrease new line thickness
     Shift+T - Increase new line thickness
           X - Pen tool
     	   S - Select tool
     	   U - Zoom in tool
     	   D - Zoom out tool
     	   R - Reset zoom
      Delete - Delete selected lines

unsigned char key - user entered key
int x - Mouse x.
int y - Mouse y.
**/
void keyControl(unsigned char key,int x,int y){
    switch (key){
        case 'X': case 'x'://Pen tool
            various&=~(1<<9);
            various&=~(1<<8);
            prevX=prevY=-1;
            deselectLines(listHead);
            glutPostRedisplay();
        break;
        
        case 'S': case 's': //Select Tool
            various&=~(1<<9);
            various|=(1<<8);
            prevX=prevY=-1;
        break;
        
        case 'U': case 'u': //Zoom in
            various|=(1<<9);
            various&=~(1<<8);
            prevX=prevY=-1;
            deselectLines(listHead);
            glutPostRedisplay();
        break;
        
        case 'd': case 'D': //Zoom out
            various|=(1<<9);
            various|=(1<<8);
            prevX=prevY=-1;
            deselectLines(listHead);
            glutPostRedisplay();
        break;
        
        case 127://Delete line
            listHead=deleteSelectedLines(listHead);
            glutPostRedisplay();
        break;
        
        case 'r': case 'R'://Reset Zoom level
            zoom(-1,x,y);
        break;
        
        case 'T'://Thicken Lines
            if (drawLine->thickness<=0xF){
                drawLine->thickness++;
            }
        break;
        
        case 't'://Thin Lines
            if (drawLine->thickness>1){
                drawLine->thickness--;
            }
        break;
        
        case 'H': case 'h'://Help toggle
            //various^=1<<3;
            //glutPostRedisplay();
            printHelp();
        break;
        
        case 'p': case 'P'://Toggle endpoints
            various^=1<<2;
            glutPostRedisplay();
        break;
        
        case 19://Save/Save as
            if (glutGetModifiers()&GLUT_ACTIVE_SHIFT){ //Save as if shift is held
                fname[0]='\0';
            }
            save();
        break;
        
        case 14://New
            listHead=NULL;
            fname="";
            fname[0]='\0';
            clearDrawLine();
            glutPostRedisplay();
        break;
        
        case 15://Load
            fname="";
            fname[0]='\0';
            textboxInit(1);
        break;
        
        case 'q': case 'Q': case 27: //Quit
            glutDestroyWindow(1);
            exit(0);
        break;
    }
}

/**
printHelp();
Prints instructions for the user.
**/
void printHelp(){
    printf("To draw a line, select the pen tool using the toolbar or\n");
    printf("the x key. With the pen tool selected, position the pointer\n");
    printf("at the location desired for end of the line, click and drag\n");
    printf("to the second position, then release the mouse button.\n");
    printf("\n");
    printf("To select a line, select the selection tool using the\n");
    printf("toolbar or the s key. Then, left click near a line to\n");
    printf("select the whole line. Hold shift to select multiple lines.\n");
    printf("Click and drag to move all selected lines.\n");
    printf("\n");
    printf("To move an endpoint, use the selection tool, then right\n");
    printf("click near an enpoint and drag to move it.\n");
    printf("\n");
    printf("     	   H - Redisplay this help\n");
    printf("	       P - Toggle display of big line endpoints\n");
    printf("       Q/Esc - Terminate the program\n");
    printf("      ctrl+S - Save the drawing\n");
    printf("Shift+ctrl+S - Save the drawing to a new name\n");
    printf("      Ctrl+O - Open a drawing\n");
    printf("      Ctrl+N - Clear drawing\n");
    printf("     	   t - Decrease new line thickness\n");
    printf("     Shift+T - Increase new line thickness\n");
    printf("           X - Pen tool\n");
    printf("     	   S - Select tool\n");
    printf("     	   U - Zoom in tool\n");
    printf("     	   D - Zoom out tool\n");
    printf("     	   R - Reset zoom\n");
    printf("      Delete - Delete selected lines\n");
}
int main(int argc,char** argv){
    init(argc,argv);
    glutMainLoop();
}
