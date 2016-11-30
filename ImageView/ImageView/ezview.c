//
//  main.c
//  ImageView
//
//  Created by jr2339 on 11/29/16.
//  Copyright © 2016 jr2339. All rights reserved.
//



#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
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

/**
 * Vertex Struct
 */
typedef struct {
    float position[3];
    float color[4];
    float texcords[2];
} Vertex;

/**
 * A simple set of vertieces that define a square with correctly mapped texture cords
 */
const Vertex Vertices[] = {
    {{1, -1, 0}, {1, 1, 1, 1}, {1, 1}},
    {{1, 1, 0}, {1, 1, 1, 1}, {1, 0}},
    {{-1, 1, 0}, {1, 1, 1, 1}, {0, 0}},
    {{-1, -1, 0}, {1, 1, 1, 1}, {0, 1}}
};

/**
 * Specifies the connections between vertices
 */
const GLubyte Indices[] = {
    0, 1, 2,
    2, 3, 0
};

/**
 * The vertex shader for the applciation, this is where almost all the
 * action happens in reguards to transformations of the image. We simply
 * pass values for Scale, Translation, Shear, and Rotation into here and
 * the shader performs the appropriate matrix operations to transform the
 * displayed underlying geometry.
 */
char* vertex_shader_src =
    "attribute vec4 Position;\n"
    "attribute vec4 SourceColor;\n"
    "attribute vec2 SourceTexcoord;\n"
    "uniform vec2 Scale;\n"
    "uniform vec2 Translation;"
    "uniform vec2 Shear;\n"
    "uniform float Rotation;\n"
    "varying vec4 DestinationColor;\n"
    "varying vec2 DestinationTexcoord;\n"
    "mat4 RotationMatrix = mat4( cos(Rotation), -sin(Rotation), 0.0, 0.0,\n"
    "                            sin(Rotation),  cos(Rotation), 0.0, 0.0,\n"
    "                            0.0,            0.0,           1.0, 0.0,\n"
    "                            0.0,            0.0,           0.0, 1.0 );\n"
    "\n"
    "mat4 TranslationMatrix = mat4(1.0, 0.0, 0.0, Translation.x,\n"
    "                              0.0, 1.0, 0.0, Translation.y,\n"
    "                              0.0, 0.0, 1.0, 0.0,\n"
    "                              0.0, 0.0, 0.0, 1.0 );\n"
    "\n"
    "mat4 ScaleMatrix = mat4(Scale.x, 0.0,     0.0, 0.0,\n"
    "                        0.0,     Scale.y, 0.0, 0.0,\n"
    "                        0.0,     0.0,     1.0, 0.0,\n"
    "                        0.0,     0.0,     0.0, 1.0 );\n"
    "\n"
    "mat4 ShearMatrix = mat4(1.0,     Shear.x, 0.0, 0.0,\n"
    "                        Shear.y, 1.0,     0.0, 0.0,\n"
    "                        0.0,     0.0,     1.0, 0.0,\n"
    "                        0.0,     0.0,     0.0, 1.0 );\n"
    "\n"
    "void main(void) {\n"
    "    DestinationColor = SourceColor;\n"
    "    DestinationTexcoord = SourceTexcoord;\n"
    "    gl_Position = Position*ScaleMatrix*ShearMatrix*RotationMatrix*TranslationMatrix;\n"
    "}";


/**
 * The fragment shader for the application. Handles actually mapping the
 * input image onto the geometry.
 */
char* fragment_shader_src =
    "varying vec4 DestinationColor;\n"
    "varying vec2 DestinationTexcoord;\n"
    "uniform sampler2D Texture;\n"
    "\n"
    "void main(void) {\n"
    "    gl_FragColor = texture2D(Texture, DestinationTexcoord) * DestinationColor;\n"
    "}";

/**
 * Compile the specified shader, provides output and checks for errors
 * along the way.
 * @param shader_type - The OpenGL Shader Type
 * @param shader_src - The shader in string form
 * @return The compiled shader id
 */
GLint simple_shader(GLint shader_type, char* shader_src) {
    GLint compile_success = 0;
    
    // Generate a new shader to work with
    GLuint shader_id = glCreateShader(shader_type);
    
    // Tell the shader where the source is
    glShaderSource(shader_id, 1, &shader_src, 0);
    
    // Print the shader before we compile it
    printf("===Compiling Shader===\n");
    printf("%s\n", shader_src);
    printf("======================\n");
    
    // Actually compile the shader
    glCompileShader(shader_id);
    
    // Check the status of the compile
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_success);
    
    // If it failed print an error
    if (compile_success == GL_FALSE) {
        GLchar message[256];
        glGetShaderInfoLog(shader_id, sizeof(message), 0, &message[0]);
        printf("glCompileShader Error: %s\n", message);
        exit(1);
    }
    
    return shader_id;
}

/**
 * Start the OpenGL program, compile the shaders, and link the program
 * @return
 */
int simple_program() {
    
    GLint link_success = 0;
    
    // Generate a new program to work with
    GLint program_id = glCreateProgram();
    // Compile the shaders
    GLint vertex_shader = simple_shader(GL_VERTEX_SHADER, vertex_shader_src);
    GLint fragment_shader = simple_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
    
    // Attach the shaders to the program
    glAttachShader(program_id, vertex_shader);
    glAttachShader(program_id, fragment_shader);
    
    // Link the program
    glLinkProgram(program_id);
    
    // Check the link status
    glGetProgramiv(program_id, GL_LINK_STATUS, &link_success);
    
    // If it failed print an error
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

// Define variables to hold our current Scale, Shear, Translation, and Rotation states
float ScaleTo[] = { 1.0, 1.0 };
float Scale[] = { 1.0, 1.0 };
float ShearTo[] = { 0.0, 0.0 };
float Shear[] = { 0.0, 0.0 };
float TranslationTo[] = { 0.0, 0.0 };
float Translation[] = { 0, 0 };
float RotationTo = 0;
float Rotation = 0;


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
        switch (key) {
                // Scale up the whole image
            case GLFW_KEY_UP:
                ScaleTo[0] += 0.5;
                ScaleTo[1] += 0.5;
                break;
                // Scale down the whole image
            case GLFW_KEY_DOWN:
                ScaleTo[0] -= 0.5;
                ScaleTo[1] -= 0.5;
                if (ScaleTo[0] < 0)
                    ScaleTo[0] = 0;
                if (ScaleTo[1] < 0)
                    ScaleTo[1] = 0;
                break;
                // Scale up in the Y direction
            case GLFW_KEY_T:
                ScaleTo[1] += 0.5;
                break;
                // Scale down in the Y direction
            case GLFW_KEY_G:
                ScaleTo[1] -= 0.5;
                if (ScaleTo[1] < 0)
                    ScaleTo[1] = 0;
                break;
                // Scale up in the X direction
            case GLFW_KEY_H:
                ScaleTo[0] += 0.5;
                break;
                // Scale down in the X direction
            case GLFW_KEY_F:
                ScaleTo[0] -= 0.5;
                if (ScaleTo[0] < 0)
                    ScaleTo[0] = 0;
                break;
                // Translate down in the X direction
            case GLFW_KEY_A:
                TranslationTo[0] -= 0.5;
                break;
                // Translate up in the X direction
            case GLFW_KEY_D:
                TranslationTo[0] += 0.5;
                break;
                // Translate down in the Y direction
            case GLFW_KEY_S:
                TranslationTo[1] -= 0.5;
                break;
                // Translate up in the Y direction
            case GLFW_KEY_W:
                TranslationTo[1] += 0.5;
                break;
                // Add rotation
            case GLFW_KEY_E:
                RotationTo += 0.1;
                break;
                // Subtract rotation
            case GLFW_KEY_Q:
                RotationTo -= 0.1;
                break;
                // Shear up in the X direction
            case GLFW_KEY_J:
                ShearTo[0] += 0.1;
                break;
                // Shear down in the X direction
            case GLFW_KEY_L:
                ShearTo[0] -= 0.1;
                break;
                // Shear up in the Y direction
            case GLFW_KEY_I:
                ShearTo[1] += 0.1;
                break;
                // Shear down in the Y direction
            case GLFW_KEY_K:
                ShearTo[1] -= 0.1;
                break;
                // Reset all values to their original
            case GLFW_KEY_R:
                ScaleTo[0] = 1.0;
                ScaleTo[1] = 1.0;
                ShearTo[0] = 0.0;
                ShearTo[1] = 0.0;
                TranslationTo[0] = 0.0;
                TranslationTo[1] = 0.0;
                RotationTo = 0;
                break;
        }
}


void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Scale the image by some portion of the amount scrolled in the Y direction
    ScaleTo[0] += yoffset * 0.5;
    ScaleTo[1] += yoffset * 0.5;
    
    if (ScaleTo[0] < 0)
        ScaleTo[0] = 0;
    if (ScaleTo[1] < 0)
        ScaleTo[1] = 0;
}


void tween(float *currentValues, float *newValues, int totalEntries)
{
    for (totalEntries--; totalEntries >= 0; totalEntries--)
        currentValues[totalEntries] += (newValues[totalEntries] - currentValues[totalEntries]) * 0.1;
}

/**
 * The main enchilada, do all the things!
 */
int main (int argc, char *argv[]) {
    // Check input arguments
    if (argc != 2) {
        fprintf(stderr, "Error: Not enough arguments provided\n");
        //show_help();
        return 1;
    }
    
    // Capture filename to load
    char *inputFname = argv[1];
    
    // Attempt to load the specified image
    //Image image;
    /*
    if (load_image(&image, inputFname) != 0) {
        fprintf(stderr, "An error occurred loading the specified source file.\n");
        exit(1);
    }
    */
    // Define GLFW variables
    GLint program_id;
    GLuint color_slot;
    GLuint position_slot;
    GLuint texcoord_slot;
    GLuint scale_slot;
    GLuint translation_slot;
    GLuint rotation_slot;
    GLuint shear_slot;
    GLuint vertex_buffer;
    GLuint index_buffer;
    GLuint tex;
    
    // Set the GLFW error callback
    glfwSetErrorCallback(error_callback);
    
    // Initialize GLFW library
    if (!glfwInit())
        return -1;
    
    // Setup GLFW window
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    
    // Create a fancy window name that has the name of the file being displayed
    char windowName[128];
    snprintf(windowName, sizeof windowName, "ezview - '%s'", inputFname);
    
    // Create and open a window
    window = glfwCreateWindow(640,
                              480,
                              windowName,
                              NULL,
                              NULL);
    
    // Make sure the window was created correctly
    if (!window) {
        glfwTerminate();
        printf("glfwCreateWindow Error\n");
        exit(1);
    }
    
    glfwMakeContextCurrent(window);
    
    program_id = simple_program();
    
    glUseProgram(program_id);
    
    // Configure all the shader slots
    position_slot = glGetAttribLocation(program_id, "Position");
    color_slot = glGetAttribLocation(program_id, "SourceColor");
    texcoord_slot = glGetAttribLocation(program_id, "SourceTexcoord");
    scale_slot = glGetUniformLocation(program_id, "Scale");
    translation_slot = glGetUniformLocation(program_id, "Translation");
    rotation_slot = glGetUniformLocation(program_id, "Rotation");
    shear_slot = glGetUniformLocation(program_id, "Shear");
    glEnableVertexAttribArray(position_slot);
    glEnableVertexAttribArray(color_slot);
    glEnableVertexAttribArray(texcoord_slot);
    
    // Create Buffer
    glGenBuffers(1, &vertex_buffer);
    
    // Map GL_ARRAY_BUFFER to this buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    
    // Send the data
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
    
    glGenBuffers(1, &index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
    
    int bufferWidth, bufferHeight;
    glfwGetFramebufferSize(window, &bufferWidth, &bufferHeight);
    
    // Configure the texture
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width, image.height, 0, GL_RGB, GL_FLOAT, image.pixmap);
    
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
    
    glVertexAttribPointer(texcoord_slot,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Vertex),
                          (GLvoid*) (sizeof(float) * 7));
    
    // Setup callbacks for events
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    // Repeat
    while (!glfwWindowShouldClose(window)) {
        
        // Tween values
        tween(Scale, ScaleTo, 2);
        tween(Translation, TranslationTo, 2);
        tween(Shear, ShearTo, 2);
        tween(&Rotation, &RotationTo, 1);
        
        // Send updated values to the shader
        glUniform2f(scale_slot, Scale[0], Scale[1]);
        glUniform2f(translation_slot, Translation[0], Translation[1]);
        glUniform2f(shear_slot, Shear[0], Shear[1]);
        glUniform1f(rotation_slot, Rotation);
        
        // Clear the screen
        glClearColor(0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glViewport(0, 0, bufferWidth, bufferHeight);
        
        // Draw everything
        glDrawElements(GL_TRIANGLES,
                       sizeof(Indices) / sizeof(GLubyte),
                       GL_UNSIGNED_BYTE, 0);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Finished, close everything up
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
