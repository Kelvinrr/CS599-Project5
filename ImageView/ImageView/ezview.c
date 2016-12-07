#include <OpenGL/gl.h>
#include <GLFW/glfw3.h>
#include "linmath.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef struct {
  float Position[2];
  float TexCoord[2];
} Vertex;

typedef struct {
    unsigned char r, g, b;
}Pixel;

Vertex vertexes[] = {
    {{1, -1}, {0.99999, 0.99999}},
    {{1, 1},  {0.99999, 0}},
    {{-1, 1}, {0, 0}},
    {{-1, 1}, {0, 0}},
    {{-1, -1}, {0, 0.99999}},
    {{1, -1}, {0.99999, 0.99999}}
};

const double pi = 3.1415926535897;
float rotation = 0;
float scale = 1;
float translate_x = 0;
float translate_y = 0;
float shear_x = 0;
float shear_y = 0;

static const char* vertex_shader_text =
    "uniform mat4 MVP;\n"
    "attribute vec2 TexCoordIn;\n"
    "attribute vec2 vPos;\n"
    "varying vec2 TexCoordOut;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
    "    TexCoordOut = TexCoordIn;\n"
    "}\n";

static const char* fragment_shader_text =
    "varying vec2 TexCoordOut;\n"
    "uniform sampler2D Texture;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = texture2D(Texture, TexCoordOut);\n"
    "}\n";
/*===============================================================*/
//void read_p3(Pixel *buffer, FILE *input_file, int width, int height);
//void read_p6(Pixel *buffer, FILE *input_file, int width, int height);
/*===============================================================*/
//===============================Code From Project1==================================================//
void read_p3(Pixel *buffer, FILE *input_file, int width, int height){
    //fgetc() and atoi() to read and convert ascii
    int current_read;
    int red, green, blue;
    int size = width * height;
    for(int i = 0; i < size; i++){
        current_read = fgetc(input_file);
        while(current_read  == ' ' || current_read  == '\n'){
            current_read = fgetc(input_file);
        }
        ungetc(current_read, input_file);
        fscanf(input_file, "%d %d %d", &red, &green, &blue);
        buffer[i].r = red;
        buffer[i].g = green;
        buffer[i].b = blue;
    }
}

void read_p6(Pixel *buffer, FILE *input_file, int width, int height){
    int size = width * height;
    for(int i = 0; i < size; i++){
        fread(&buffer[i].r, 1, 1, input_file);
        fread(&buffer[i].g, 1, 1, input_file);
        fread(&buffer[i].b, 1, 1, input_file);
    }
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    //Add keys for transformations here
    if (key == GLFW_KEY_Q && action == GLFW_PRESS) //ROTATE CCW
        rotation += 90*pi/180;
    if (key == GLFW_KEY_E && action == GLFW_PRESS) //ROTATE CW
        rotation -= 90*pi/180;
    if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS)//
        scale *= 2;
    if (key == GLFW_KEY_MINUS && action == GLFW_PRESS)
        scale *= .5;
    if (key == GLFW_KEY_UP && action == GLFW_PRESS)  //Translate Up
        translate_y += .1;
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) //Translate Down
        translate_y -= .1;
    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) //Translate Right
        translate_x += .1;
    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) //Translate Left
        translate_x -= .1;
    if (key == GLFW_KEY_D && action == GLFW_PRESS) //Shear Up
        shear_y += .1;
    if (key == GLFW_KEY_A && action == GLFW_PRESS) //Shear Down
        shear_y -= .1;
    if (key == GLFW_KEY_W && action == GLFW_PRESS) //Shear Right
        shear_x += .1;
    if (key == GLFW_KEY_S && action == GLFW_PRESS) //Shear Left
        shear_x -= .1;
    //TODO: SCALE, SHEAR
}

void glCompileShaderOrDie(GLuint shader) {
  GLint compiled;
  glCompileShader(shader);
  glGetShaderiv(shader,
		GL_COMPILE_STATUS,
		&compiled);
  if (!compiled) {
    GLint infoLen = 0;
    glGetShaderiv(shader,
		  GL_INFO_LOG_LENGTH,
		  &infoLen);
    char* info = malloc(infoLen+1);
    GLint done;
    glGetShaderInfoLog(shader, infoLen, &done, info);
    printf("Unable to compile shader: %s\n", info);
    exit(1);
  }
}

/*
unsigned char image[] = {
  255, 0, 0, 255,
  255, 0, 0, 255,
  255, 0, 0, 255,
  255, 0, 0, 255,

  0, 255, 0, 255,
  0, 255, 0, 255,
  0, 255, 0, 255,
  0, 255, 0, 255,

  0, 0, 255, 255,
  0, 0, 255, 255,
  0, 0, 255, 255,
  0, 0, 255, 255,

  255, 0, 255, 255,
  255, 0, 255, 255,
  255, 0, 255, 255,
  255, 0, 255, 255
};
*/
int main(int argc, char *argv[])
{
    GLFWwindow* window;
    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location;
    /*=======================================================================*/
     int image_width, image_height, max_color, image_type, image_format;
     FILE *input_file;
     Pixel *image;
    /*=======================================================================*/
    glfwSetErrorCallback(error_callback);
    /*=======================================================================*/
    if (argc !=2) {
        fprintf(stderr, "ERROR: Incorrect number of arguments.");
        exit(1);
    }
    input_file = fopen(argv[1], "r");
    if(input_file == NULL){
        fprintf(stderr, "ERROR: Failed to open input file.\r\n");
        return EXIT_FAILURE;
    }
    //Check the type of the image
    image_type = getc(input_file);
    if(image_type != 'P'){
        fprintf(stderr, "ERROR: Input file is not in PPM format.\r\n");
    }
    image_format = getc(input_file);
    if(!(image_format != '6'|| image_format != '3')){
        fprintf(stderr, "ERROR: Unsupported image type. Please provide a PPM image in either P3 or P6 format. Input given: %c.\r\n", image_format);
    }
    image_type = getc(input_file); //should get newline
    image_type = getc(input_file);//should get either a comment character or number
    if (image_type == '#'){
        while(image_type != '\n'){
            image_type = getc(input_file);
        }
        printf("%c", image_type);
    }
    else{  //if there wasn't a comment, we want to go back one character so we're at the start of the line.
        ungetc(image_type, input_file);
    }
    //get width, height, and max color value
    fscanf(input_file, "%d %d\n%d\n", &image_width, &image_height, &max_color);
    if(max_color >= 256){
        fprintf(stderr, "ERROR: Multi-byte samples not supported.\r\n");
        return EXIT_FAILURE;
    }
    //allocate memory for all the pixels
    image = (Pixel *)malloc(image_width*image_height*sizeof(Pixel));
    if(image_format == '3'){
        read_p3(image, input_file, image_width, image_height);
    }
    else if(image_format == '6'){
        read_p6(image, input_file, image_width, image_height);
    }
    else{
        fprintf(stderr, "ERROR: Unknown format.");
        return EXIT_FAILURE;
    }
    fclose(input_file);
    /*=======================================================================*/
    if (!glfwInit())
        exit(EXIT_FAILURE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    // gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);

    // NOTE: OpenGL error checks have been omitted for brevity

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexes), vertexes, GL_STATIC_DRAW);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShaderOrDie(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShaderOrDie(fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    // more error checking! glLinkProgramOrDie!

    mvp_location = glGetUniformLocation(program, "MVP");
    assert(mvp_location != -1);

    vpos_location = glGetAttribLocation(program, "vPos");
    assert(vpos_location != -1);

    GLint texcoord_location = glGetAttribLocation(program, "TexCoordIn");
    assert(texcoord_location != -1);

    GLint tex_location = glGetUniformLocation(program, "Texture");
    assert(tex_location != -1);

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location,
			  2,
			  GL_FLOAT,
			  GL_FALSE,
                          sizeof(Vertex),
			  (void*) 0);

    glEnableVertexAttribArray(texcoord_location);
    glVertexAttribPointer(texcoord_location,
			  2,
			  GL_FLOAT,
			  GL_FALSE,
                          sizeof(Vertex),
			  (void*) (sizeof(float) * 2));
    
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB,
		 GL_UNSIGNED_BYTE, image);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    glUniform1i(tex_location, 0);

    while (!glfwWindowShouldClose(window))
    {
        float ratio;
        int width, height;
        //mat4x4 m, p, mvp;
        mat4x4 r, h, s, t, rh, rhs, mvp; //matrices for each transformation and intermediate values

        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        /*
        mat4x4_identity(m);
        mat4x4_rotate_Z(m, m, (float) glfwGetTime());
        mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
        mat4x4_mul(mvp, p, m);
        */
        /*=================================================*/
        //RHS T
        mat4x4_identity(r);
        mat4x4_rotate_Z(r, r, rotation); //angle of rotation
        mat4x4_identity(h);
        h[0][1] = shear_x;
        h[1][0] = shear_y;
        
        mat4x4_identity(s); //NOT WORKING AS INTENDED
        s[0][0] = s[0][0]*scale;
        s[1][1] = s[1][1]*scale;
        
        mat4x4_identity(t);
        mat4x4_translate(t, translate_x, translate_y, 0);
        
        mat4x4_mul(rh, r, h); //R*H
        mat4x4_mul(rhs, rh, s);//R*H*S
        mat4x4_mul(mvp, rhs, t);//R*H*S*T

        /*=================================================*/
        glUseProgram(program);
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}



