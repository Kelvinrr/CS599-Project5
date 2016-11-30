//
//  main.c
//  ImageView
//
//  Created by jr2339 on 11/29/16.
//  Copyright © 2016 jr2339. All rights reserved.
//

#define GLFW_DLL 1
#define GL_GLEXT_PROTOTYPES

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <GLFW/glfw3.h>
typedef struct Image
{
    int width;
    int height;
    u_char *data; // color
    int magic_number;
    int depth;
    char *tupltype;
    int maxval;
}Image;

/*=============================================================================*/
Image *ImageCreate(int width, int height,int magic_number);
Image *readPPMHeader(FILE *f_source, Image *image);
Image *readPAMHeader(FILE *f_source, Image *image);
Image *ImageRead(const char *filename);
void ImageWrite(Image *image, const char *filename,int format);
/*=============================================================================*/

int readMagicNumber(FILE *fp) {
    int magic_number;
    if (!fscanf(fp, "P%d\n", &magic_number)){
        fprintf(stderr,"file is not in PPM format, we can't read it");
        exit(1);
    }
    return magic_number;
}
/**************************************************************************************************************
 **************************************************************************************************************/
// readPPMHeader help us to detemine which format is our source image
Image *readPPMHeader(FILE *f_source, Image *buffer){
    char ch;
    /*skip the comments*/
    ch = getc(f_source); // ch is int
    /*
     gets the next character (an unsigned char) from the specified
     stream and advances the position indicator for the stream.
     */
    while(ch =='#'){
        do{
            ch=getc(f_source);
        }
        while(ch!='\n'); //read to the end of the line
        ch=getc(f_source);
    }
    if(!isdigit(ch)){
        fprintf(stderr,"can't read header information from PPM format\n");
    }
    else{
        ungetc(ch, f_source); //put that digital back
    }
    
    /*default tupletype and depth for P3 is RGB and 3*/
    buffer->tupltype = "RGB";
    buffer->depth = 3;
    //read the width, height,amd maximum value for a pixel
    fscanf(f_source, "%d %d %d\n",&buffer->width,&buffer->height,&buffer->maxval);
    buffer->data = (u_char *)malloc(buffer->width*buffer->height*4);// G, R, B three colors
    if(buffer->maxval >= 65336 ||buffer->maxval <= 0){
        fprintf(stderr,"image is not ture-color(8byte), read failed\n");
        exit(1);
    }
    return buffer;
    
}
/**************************************************************************************************************
 **************************************************************************************************************/
Image *readPAMHeadr(FILE *f_source, Image *buffer){
    char array1[1024];
    int array2;
    char array3[1024];
    while(1){
        fscanf(f_source, "%s", array1);
        if(strcmp(array1, "ENDHDR") == 0){   // we read the whole header
            buffer->data = (u_char *)malloc(buffer->width*buffer->height*4);// G, R, B three colors
            return buffer;
        }else if(strcmp(array1, "TUPLTYPE") == 0){ // if it is tupltype, we put value to it
            fscanf(f_source, "%s", array3);
            buffer->tupltype = array3;
        }
        else{
            fscanf(f_source, "%d", &array2);      // read width, height and depth , maxvalue at here
            if(strcmp(array1, "WIDTH") == 0){
                buffer->width = array2;
            }else if(strcmp(array1, "HEIGHT") == 0){
                buffer->height = array2;
            }else if(strcmp(array1, "DEPTH") == 0){
                buffer->depth = array2;
            }else if(strcmp(array1, "MAXVAL") == 0){
                buffer->maxval = array2;
            }
        }
    }
    
}
/**************************************************************************************************************
 **************************************************************************************************************/
Image *ImageRead(const char *filename){
    FILE *fp = fopen(filename,"r");
    if(!fp){
        fprintf(stderr,"can't open file for reading \n");
        exit(1);
    }
    Image *buffer = (Image *)malloc(sizeof(Image));
    if(!buffer){
        fprintf(stderr,"Can't allocate memory for new image");
        exit(1);
    }
    buffer->magic_number = readMagicNumber(fp);
    buffer->data = (u_char *)malloc(buffer->width*buffer->height*4);// G, R, B three colors and A
    if(buffer->magic_number==3){
        readPPMHeader(fp,buffer);
        buffer->data = (u_char *)malloc(buffer->width*buffer->height*4);
        int i,j, pixel[4];
        //In P3 file, we use nested loop, to copy image content to buffer
        for(i=0;i<buffer->height;i++){
            for(j=0;j<buffer->width;j++){
                fscanf(fp, "%d\n",pixel);
                buffer->data[i*buffer->width*4+4*j] = *pixel;//R
                fscanf(fp, "%d\n",pixel);
                buffer->data[i*buffer->width*4+4*j+1] = *pixel;//G
                fscanf(fp, "%d\n",pixel);
                buffer->data[i*buffer->width*4+4*j+2] = *pixel;//B
                buffer->data[i*buffer->width*4+4*j+3] = 255;  //A at here default value is 255
            }
        }
    }
    else if(buffer->magic_number==6){
        readPPMHeader(fp,buffer);
        buffer->data = (u_char *)malloc(buffer->width*buffer->height*4);
        int size1 = buffer->width * buffer->height*4;
        int PPM_size = buffer->height*buffer->width*3;//give the times for our for loop
        char c='A';//init
        int k=0;
        // for each llop we copy R, G, B to the buffer and let A =255
        for(int m=0;m<PPM_size;m+=3){
            fread(&c, 1, 1, fp);
            buffer->data[k] = c;
            k += 1;
            fread(&c, 1, 1, fp);
            buffer->data[k] = c;
            k += 1;
            fread(&c, 1, 1, fp);
            buffer->data[k] = c;
            k += 1;
            buffer->data[k] = 255;
            k += 1;
        }
        
        if (k!= size1) {
            printf("n is %d, size is %d \n",k,size1);
            fprintf(stderr,"cannot read image datat from file \n");
            exit(1);
        }
        
    }
    else if(buffer->magic_number == 7){
        size_t num2;
        readPAMHeadr(fp,buffer);
        buffer->data = (u_char *)malloc(buffer->width*buffer->height*4);
        char ch = getc(fp); // ch is int
        if (ch != '\n') {
            ungetc(ch, fp); //put that char back
        }
        int size2 = buffer->width * buffer->height*4;
        if (strcmp(buffer->tupltype, "RGB_ALPHA") == 0){
            num2 = fread((void *) buffer->data, 1, (size_t) size2, fp);
            if (num2 != size2) {
                fprintf(stderr,"cannot read image datat from file \n");
                exit(1);
            }
            
        }else if (strcmp(buffer->tupltype, "RGB")==0){
            int size3 = buffer->height*buffer->width*3;
            char c='A';//init
            int k=0;    // each time k incrsement 3
            for(int m=0;m<size3;m+=3){
                fread(&c, 1, 1, fp);   // in tis 3 times we put RGB
                buffer->data[k] = c;   // into the buffer, and let the
                k += 1;                //4th one eaual 255
                fread(&c, 1, 1, fp);
                buffer->data[k] = c;
                k += 1;
                fread(&c, 1, 1, fp);
                buffer->data[k] = c;
                k += 1;
                buffer->data[k] = 255;
                k += 1;
            }
            if (k != size2) {
                fprintf(stderr,"cannot read image datat from file \n");
                exit(1);
            }
        }
    }
    return buffer;
}
/**************************************************************************************************************
 **************************************************************************************************************/
void ImageWrite(Image *buffer, const char *filename,int format){
    size_t num2;
    int size = buffer->width * buffer->height * 4;
    FILE *f_des = fopen(filename, "w");
    if (!f_des){
        fprintf(stderr,"cannot open file for writing");
    }
    if(format==6){
        fprintf(f_des, "P%d\n%d %d\n%d\n",format,buffer->width, buffer->height, buffer->maxval);
        for(int i=1; i<size+1;i++){    // for each for slots we skip it,
            char ch=buffer->data[i-1];
            if (i%4 !=0) {
                fwrite(&ch, 1, 1, f_des);  // which means we skip A at here
            }
        }
    }
    else if(format==3){
        fprintf(f_des, "P%d\n%d %d\n%d\n",format,buffer->width, buffer->height, buffer->maxval);
        int i,j;
        for(i=0;i<buffer->height;i++){
            for(j=0;j<buffer->width;j++){
                fprintf(f_des, "%d\n",buffer->data[i*buffer->width*4+4*j]); // We got R and put in to the Buffer
                fprintf(f_des, "%d\n",buffer->data[i*buffer->width*4+4*j+1]);// We got G and put in to the Buffer
                fprintf(f_des, "%d\n",buffer->data[i*buffer->width*4+4*j+2]);// We got B and put in to the Buffer
                //Skip the A
            }
            fprintf(f_des, "\n");
        }
    }
    else if(format==7){
        fprintf(f_des,"P%d\nWIDTH %d\nHEIGHT %d\nDEPTH %d\nMAXVAL %d\nTUPLTYPE %s\nENDHDR\n",format,buffer->width,buffer->height,buffer->depth,buffer->maxval,buffer->tupltype);
        if (strcmp(buffer->tupltype, "RGB_ALPHA") == 0) {
            num2 = fwrite((void *) buffer->data, 1, (size_t) size, f_des);// We use fwrite
        }else if (strcmp(buffer->tupltype, "RGB")==0){
            for(int i=1; i<size+1;i++){   // for each four slots we skip it
                char ch=buffer->data[i-1];
                if (i%4 !=0) {
                    fwrite(&ch, 1, 1, f_des);
                }
            }
        }
        fclose(f_des);
    }
}

/*=============================================================================*/
GLFWwindow* window;
typedef struct {
    float position[3];
    float color[4];
    float texcoord[2];
} Vertex;

const Vertex Vertices[] = {
    {{1, -1, 0}, {1, 0, 0, 1}, {1,0}}, //Bottom Right
    {{1, 1, 0}, {0, 1, 0, 1}, {1,1}},  //Top Right
    {{-1, 1, 0}, {0, 0, 1, 1}, {0,1}}, //Top Left
    {{-1, -1, 0}, {0, 0, 0, 1}, {0,0}} //Bottom Left
};

const GLubyte Indices[] = {
    0, 1, 2,
    2, 3, 0
};


char* vertex_shader_src =
    "attribute vec4 Position;\n"
    "attribute vec4 SourceColor;\n"
    "\n"
    "varying vec4 DestinationColor;\n"
    "\n"
    "void main(void) {\n"
    "    DestinationColor = SourceColor;\n"
    "    gl_Position = Position;\n"
    "}\n";


char* fragment_shader_src =
    "varying lowp vec4 DestinationColor;\n"
    "\n"
    "void main(void) {\n"
    "    gl_FragColor = DestinationColor;\n"
    "}\n";


GLint simple_shader(GLint shader_type, const char* shader_src) {
    
    GLint compile_success = 0;
    
    int shader_id = glCreateShader(shader_type);
    
    glShaderSource(shader_id, 1, &shader_src, 0);
    
    glCompileShader(shader_id);
    
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_success);
    
    if (compile_success == GL_FALSE) {
        GLchar message[256];
        glGetShaderInfoLog(shader_id, sizeof(message), 0, &message[0]);
        printf("glCompileShader Error: %s\n", message);
        exit(1);
    }
    
    return shader_id;
}


int simple_program() {
    
    GLint link_success = 0;
    
    GLint program_id = glCreateProgram();
    GLint vertex_shader = simple_shader(GL_VERTEX_SHADER, vertex_shader_src);
    GLint fragment_shader = simple_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
    
    glAttachShader(program_id, vertex_shader);
    glAttachShader(program_id, fragment_shader);
    
    glLinkProgram(program_id);
    
    glGetProgramiv(program_id, GL_LINK_STATUS, &link_success);
    
    if (link_success == GL_FALSE) {
        GLchar message[256];
        glGetProgramInfoLog(program_id, sizeof(message), 0, &message[0]);
        printf("glLinkProgram Error: %s\n", message);
        exit(1);
    }
    
    return program_id;
}


static void error_callback(int error, const char* description) {
    fputs(description, stderr);
}


int main(int argc, char *argv[]) {
    /************************************************************************************************/
    if(argc < 2){
        perror("We need more arguments");
        exit(1);//if the nunber is not 0, not access to error
    }
    else if(argc > 2){
        perror("Too many arguments");
        exit(1);//if the nunber is not 0, not access to error
    }
    const char *inputNmae = argv[1];  //input.ppm
    Image *image;
    image = ImageRead(inputNmae);
     /************************************************************************************************/
    
    
    GLint program_id, position_slot, color_slot;
    GLuint vertex_buffer;
    GLuint index_buffer;
    
    glfwSetErrorCallback(error_callback);
    
    // Initialize GLFW library
    if (!glfwInit())
        return -1;
    
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    
    // Create and open a window
    window = glfwCreateWindow(640,
                              480,
                              "ezview",
                              NULL,
                              NULL);
    
    if (!window) {
        glfwTerminate();
        printf("glfwCreateWindow Error\n");
        exit(1);
    }
    
    glfwMakeContextCurrent(window);
    
    program_id = simple_program();
    
    glUseProgram(program_id);
    
    position_slot = glGetAttribLocation(program_id, "Position");
    color_slot = glGetAttribLocation(program_id, "SourceColor");
    glEnableVertexAttribArray(position_slot);
    glEnableVertexAttribArray(color_slot);
    
    // Create Buffer
    glGenBuffers(1, &vertex_buffer);
    
    // Map GL_ARRAY_BUFFER to this buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    
    // Send the data
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
    
    glGenBuffers(1, &index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
    
    // Repeat
    while (!glfwWindowShouldClose(window)) {
        
        glClearColor(0, 104.0/255.0, 55.0/255.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glViewport(0, 0, 640, 480);
        
        glVertexAttribPointer(position_slot,
                              3,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(Vertex),
                              0);
        
        glVertexAttribPointer(color_slot,
                              4,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(Vertex),
                              (GLvoid*) (sizeof(float) * 3));
        
        glDrawElements(GL_TRIANGLES,
                       sizeof(Indices) / sizeof(GLubyte),
                       GL_UNSIGNED_BYTE, 0);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
