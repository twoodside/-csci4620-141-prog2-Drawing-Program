#include <string.h>

typedef struct Line{
    int left[2]; //Left-most vertex of the line.
    int right[2]; //Right-most vertex of the line.
    unsigned int thickness;
    unsigned char selected; //Boolean. 1 if line is selected, 0 if not.
} Line;

typedef struct LineList{
    Line *line;
    struct LineList *next;
} LineList;

//Linked List processing
LineList* addCoords(int l[2],int r[2],unsigned int thick,LineList* oldNode);
LineList* addLine(Line *newLine,LineList* oldNode);
Line* selectEndpoint(LineList* node,int x,int y);
void selectLine(LineList* node,int x,int y,int deselect);
void deselectLines(LineList *node);
LineList* deleteSelectedLines(LineList* node);
void moveSelectedLines(LineList *node, int dx, int dy);


/**
listHead=addCoords(v1,v2,thick,listHead);
Adds a Line to the linked list from the given coordinates. Adds to the front of the list. Returns the list node.
int l[2] - Left coordinates. Will be verified to be the left-most coordinates, and swapped if they aren't.
int r[2] - Right coordinates.
unsigned int thick - Line thickenss for the new line.
LineList *oldNode - Head of the list to add the new line to.

return LineList* - new head. In this case, will always return a node containing the newly created line.
**/
LineList* addCoords(int l[2],int r[2],unsigned int thick,LineList* oldNode){
    Line *newLine=malloc( sizeof(Line) );
    LineList *newNode=malloc( sizeof(LineList) );
    int *temp,i;
    //Ensure the first vertex is the leftmost.
    if (l[0]>r[0]){
        temp=l;
        l=r;
        r=temp;
    }
    
    for (i=0;i<2;i++){
        newLine->left[i]=l[i];
        newLine->right[i]=r[i];
    }
    newLine->thickness=thick;
    //newLine->selected=1;
    
    newNode->line=newLine;
    newNode->next=oldNode;
    
    return newNode;
}

/**
listHead=addCoords(v1,v2,thick,listHead);
Adds a Line to the linked list from the given line. Adds to the front of the list. Returns the list node.
Line *newLine - Line to add to the list.
LineList *oldNode - Head of the list to add the new line to.

return LineList* - new head. In this case, will always return a node containing newLine.
**/
LineList* addLine(Line *newLine,LineList* oldNode){
    LineList *newNode=malloc( sizeof(LineList) );
    int *temp;
    //Ensure the first vertex is the leftmost.
    if ( newLine->left[0] > newLine->right[0]){
        temp=newLine->left;
        newLine->left=newLine->right;
        newLine->right=temp;
    }
    
    newNode->line=newLine;
    newNode->next=oldNode;
    
    return newNode;
}


/**
temp=selectEndpoint(listHead,x,y);
Returns the first line with an endpoint within 10 px of (x,y). Returns NULL if none found within 10px.
LineList *node - The list to search through.
int x - mouse x.
int y - mouse y.

return Lne* - The first line with and point within 10 px of (x,y). NULL if none found within 10px.
**/
Line* selectEndpoint(LineList* node,int x,int y){
    LineList* origNode=node;
    while (node!=NULL){
        if ( (x-node->line->left[0])*(x-node->line->left[0]) + (y-node->line->left[1])*(y-node->line->left[1]) <=100 ){
            deselectLines(node);
            node->line->selected=1;
            origNode=deleteSelectedLines(origNode);
            break;
        }
        node=node->next;
    }
    
    if (node==NULL){
        return NULL;
    }
    
    return node->line;
}

/**
selectLine(listHead,x,y,glutGetModifiers()&GLUT_ACTIVE_SHIFT);
Sets the "selected" boolean to 1 for line(s) near to (x,y). If deselect is 1, will select multiple lines. If deselect is 0, will only select one line and will deselect all other lines.
LineList *node - Line to check for nearness.
int x - mouse x
int y - mouse y
int deselect - Inverted. If 0, will only select one line and will deselect all other lines. If 1, will select multiple lines.
**/
void selectLine(LineList* node,int x,int y,int deselect){
    float a,b,c,d,e;
    if (node==NULL){
        return;
    }
    a=abs( (x - node->line->left[0])*(node->line->right[1] - node->line->left[1]) );
    b=abs( (y - node->line->left[1])*(node->line->right[0] - node->line->left[0]) );
    c=(x-node->line->left[0])*(x-node->line->right[0]);
    d=(y-node->line->left[1])*(y-node->line->right[1]);
    
    if (   ( a*1.2>=b )
        && ( a*0.8<=b )
        && ( c<0 )
        && ( d<0 )
        && ( node->line->selected==0 )
       )
    {
        node->line->selected=1;
        if (!deselect){
            selectLine(node->next,x,y,deselect);
        }
        else{
            deselectLines(node->next);
        }
    }
    else if (
                (node->line->right[1] - node->line->left[1]<=16 && node->line->right[1] - node->line->left[1]>=-16) && //Horizontal line
                (y - node->line->left[1]<=50 && y - node->line->left[1]>=-50) &&
                (abs(c)<50 || abs(d)<50) &&
                ( node->line->selected==0 ) 
            )
    {
        node->line->selected=1;
        if (!deselect){
            selectLine(node->next,x,y,deselect);
        }
    }
    else if (
                (node->line->right[0] - node->line->left[0]<=16 && node->line->right[0] - node->line->left[0]>=-16) && //Vertical line
                (x - node->line->left[0]<=16 && x - node->line->left[0]>=-16) &&
                (abs(c)<16 || abs(d)<16) &&
                ( node->line->selected==0 )
            )
    {
        node->line->selected=1;
        if (!deselect){
            selectLine(node->next,x,y,deselect);
        }
    }
    else{
        if (!deselect){
            node->line->selected=0;
        }
        selectLine(node->next,x,y,deselect);
    }
}

/**
listHead=deleteSelectedLines(listHead);
Removes all lines whos selected parameters are 1 from the linked list.

LineList *node - Head of the list/sublist.
return LineList* - New list/sublist head.
**/
LineList* deleteSelectedLines(LineList* node){
    if (node==NULL){
        return NULL;
    }
    
    node->next=deleteSelectedLines(node->next);
    
    if (node->line->selected){
        return node->next;
    }
    return node;
}

/**
deselectLines(listHead);
Marks all lines in the list as not selected.
LineList *node - List head.
**/
void deselectLines(LineList* node){
    while (node!=NULL){
        node->line->selected=0;
        node=node->next;
    }
}

/**
moveSelectedLines(listHead,prevX-x,prevY-y);

LineList *node - List head.
int dx - Change in x by which to move all Lines.
int dy - Change in y by which to move all Lines.
**/
void moveSelectedLines(LineList *node, int dx, int dy){
    while (node!=NULL){
        if (node->line->selected){
            node->line->left[0]+=dx;
            node->line->right[0]+=dx;
            node->line->left[1]+=dy;
            node->line->right[1]+=dy;
        }
        
        node=node->next;
    }
}
