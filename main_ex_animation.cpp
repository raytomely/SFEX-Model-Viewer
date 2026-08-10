#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
//#include "freeflycamera.h"
#include "camera.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

typedef struct
{
    uint8_t x, y;
}Texcoord;

typedef struct
{
    int16_t x, y, z, pad;
}Vec3;

typedef struct
{
    Texcoord t1, t2, t3, t4;
    Vec3 v1, v2, v3;
}Triangle;

typedef struct
{
    //Vec3 v1, v2, v3, v4;
    Texcoord t1, t2, t4, t3;
    Vec3 v1, v2, v4, v3;
}Quad;

typedef struct
{
    float x, y;
}Point2D;

typedef struct
{
    Point2D t1, t2, t3;
}TriTexcoord;

typedef struct
{
    Point2D t1, t2, t3, t4;
}QuadTexcoord;

typedef struct
{
    float x, y, z;
}Vec3Float;

typedef struct
{
    Vec3Float v1, v2, v3;
}FloatTriangle;

typedef struct
{
    Vec3Float v1, v2, v3, v4;
}FloatQuad;

typedef union
{
    uint32_t children_info[2];

    struct
    {
        uint8_t children_count;
        uint8_t children_indexes[7];
    };
}BoneMap;

typedef struct
{
    unsigned int triangles_count;
    Triangle* triangles;
    unsigned int quads_count;
    Quad* quads;
    BoneMap* bone_map;
    glm::mat4 bone_mat;
    TriTexcoord* tri_texcoords;
    QuadTexcoord* quad_texcoords;
    FloatTriangle* float_triangles;
    FloatQuad* float_quads;
    FloatTriangle* triangle_normals;
    FloatQuad* quad_normals;
}Mesh;

typedef struct
{
    float x_angle, y_angle, z_angle;
}BoneRotation;

typedef struct
{
    unsigned int duration;
    unsigned int time;
    Vec3 axis_shift;
    BoneRotation** bones_rotations;
}AnimFrame;

typedef struct
{
    unsigned int frames_count;
    unsigned int index;
    AnimFrame** frames;
    unsigned int current_frame;
}Animation;

typedef struct
{
    unsigned int meshes_count;
    Mesh** meshes;
    unsigned int animations_count;
    Animation** animations;
    unsigned int current_animation;
    uint8_t used_cluts[16];
    uint32_t max_clut_index;
    uint32_t texture_height;
    GLuint texture;
    Vec3 meshes_init_pos[30];
    Vec3 shoulders_init_pos_2[2];
}Model;

typedef struct
{
    uint8_t pixel_mode;
    uint8_t has_clut;
    uint16_t *clut;
    uint32_t cluts_count;
    uint16_t clut_x, clut_y;
    uint16_t clut_width, clut_height;
    uint16_t image_x, image_y;
    uint16_t image_width, image_height, image_real_width;
    uint8_t *image_pixels;
}Tim;

typedef struct
{
    char* data;
    char* meshes_data;
    char* animations_data;
    char* texture_data;
    uint32_t meshes_data_size;
    uint32_t animations_data_size;
    uint32_t texture_data_size;
}Pac;

void draw_cube(void);
void test(void);
void quad_test(void);
void tri_normal_test(FloatTriangle* triangle);
void quad_normal_test(FloatQuad* quad);
void draw_texture(void);
void processInput(void);
void sleep(void);
GLuint loadTexture(const char * filename,bool useMipMap);
GLuint load_tim(char* data, bool useMipMap, uint32_t max_clut_index);

Pac* load_pac(char* file_path);
void free_pac(Pac* pac);

Mesh* read_mesh_quad(char* file_path);
void free_mesh_quad(Mesh* mesh);
void print_mesh_quad(Mesh* mesh);
void draw_mesh_quad(Mesh* mesh);

Mesh* read_mesh_tri(char* file_path);
void free_mesh_tri(Mesh* mesh);
void print_mesh_tri(Mesh* mesh);
void draw_mesh_tri(Mesh* mesh);

Mesh* read_mesh_2(char* file_path);
void free_mesh_2(Mesh* mesh);

Mesh* read_mesh_3(char* file_path);
Mesh* read_mesh_4(char* file_path);

Model* read_model_meshes(char* file_path);
void read_model_meshes_2(char* data, uint32_t data_size, Model* model);
void free_model_meshes(Model* model);

void read_model_animations(char* data, Model* model);
void free_model_animations(Model* model);

Model* load_model(char* pac_file_path);
void free_model(Model* model);

void load_animation_frame(Model* model, uint32_t anim_index, uint32_t frame_index);
void update_model_animation(Model* model);
void change_model_animation(Model* model, uint32_t animation_index);

void draw_mesh(Mesh* mesh);
void draw_model(Model* model, glm::mat4& model_view_matrix);

void read_bones_data(char* file_path, uint32_t offset, uint32_t length, Model* model);

void generate_normals(Model* model);

void print_axis_shifts(Model* model, uint32_t anim_index);

glm::mat4 model_transform(void);
glm::mat4 get_bone_matrix(float rot_angles[3], Vec3 *init_pos);

Vec3 meshes_init_pos[30] = {{0x00, 0x00, 0x00},  // lower torso
                            {0x00, 0x00, 0x014D},  // upper torso
                            {0x00, 0x00, 0x01B3},
                            {0x0100, 0x00, 0x011A},
                            {0x00, 0xFFFA, 0x028D},
                            {0x00, 0x05, 0x0243},  // first right hand
                            {0x00, 0x00, 0x00},  // second right hand - padding
                            {0x00, 0x00, 0x00},  // third right hand - padding
                            {0xFF00, 0x00, 0x011A},  // left shoulder
                            {0x00, 0xFFFA, 0x028D},
                            {0x00, 0x05, 0x0243},  // first left hand
                            {0x00, 0x00, 0x00},  // second left hand
                            {0x00, 0x00, 0x00},  // third left hand - padding
                            {0x00, 0x00, 0x00},  // waist
                            {0x00E6, 0x00, 0x01E6},  // right hips
                            {0x00, 0x05, 0x03A6},  // right knee
                            {0x00, 0xFFFA, 0x04F0},  // right feet
                            {0xFF19, 0x00, 0x01E6},
                            {0x00, 0x05, 0x03A6},
                            {0x00, 0xFFFA, 0x04F0}};


float meshes_rot_angles[30][3] = {{272.0f, 358.0f, 15.0f},  // lower torso
                                  {333.0f, 13.0f, 348.0f},  // upper torso
                                  {74.0f, 121.0f, 252.0f},
                                  {320.0f, 98.0f, 232.0f},
                                  {90.0f, 0.0f, 0.0f},
                                  {0.0f, 13.0f, 0.0f},  // first right hand
                                  {0.0f, 0.0f, 0.0f},  // second right hand - padding
                                  {0.0f, 0.0f, 0.0f},  // third right hand - padding
                                  {329.0f, 75.0f, 240.0f},  // left shoulder
                                  {113.0f, 0.0f, 0.0f},
                                  {12.0f, 333.0f, 359.0f},  // first left hand
                                  {0.0f, 0.0f, 0.0f},  // second left hand  - padding
                                  {0.0f, 0.0f, 0.0f},  // third left hand - padding
                                  {103.0f, 10.0f, 349.0f},  // waist
                                  {18.0f, 314.0f, 25.0f},  // right hips
                                  {303.0f, 0.0f, 0.0f},  // right knee
                                  {10.0f, 12.0f, 349.0f},  // right feet
                                  {352.0f, 7.0f, 28.0f},
                                  {326.0f, 0.0f, 0.0f},
                                  {33.0f, 343.0f, 330.0f}};


Vec3 shoulders_init_pos_2[2] = {{0x00, 0x00, 0xFE99},
                                {0x00, 0x00, 0x0166}};

float shoulders_rot_angles_2[2][3] = {{20.0f, 71.0f, 148.0f},
                                      {55.0f, 349.0f, 272.0f}};

BoneMap bones_maps[30] = {{0x00000201, 0x00000000},  // lower torso
                          {0x08040303, 0x00000000},  // upper torso
                          {0x00000000, 0x00000000},  // head
                          {0x00000501, 0x00000000},
                          {0x00000601, 0x00000000},
                          {0x00000701, 0x00000000},  // right wrist
                          {0x00000000, 0x00000000},  // right hand
                          {0x00000000, 0x00000000},  // third right hand - padding
                          {0x00000901, 0x00000000},  // left shoulder
                          {0x00000A01, 0x00000000},
                          {0x00000B01, 0x00000000},  // left wrist
                          {0x00000000, 0x00000000},  // left hand
                          {0x00000000, 0x00000000},  // third left hand - padding
                          {0x00100D02, 0x00000000},  // waist / hips
                          {0x00000E01, 0x00000000},  // right knee
                          {0x00000F01, 0x00000000},  // right ankle
                          {0x00000000, 0x00000000},  // right feet
                          {0x00001101, 0x00000000},
                          {0x00001201, 0x00000000},
                          {0x00000000, 0x00000000}};

uint32_t bones_metadata[30][2] = {{0x00, 0x13},  // character 0x00: Kairi
                                  {0x10c, 0x16},  // character 0x01: Hokuto
                                  {0x240, 0x13},  // character 0x02: Darun
                                  {0x34c, 0x15},  // character 0x03: D.Dark
                                  {0x474, 0x17},  // character 0x04: Pullum
                                  {0x5b8, 0x13},  // character 0x05: Blair
                                  {0x6e0, 0x13},  // character 0x06: Skullomania
                                  {0x7ec, 0x13},  // character 0x07: Allen
                                  {0x904, 0x14},  // character 0x08: C.Jack
                                  {0x00, 0x13},  // character 0x09: Guile
                                  {0x904, 0x13},  // character 0x0A: M.Bison
                                  {0x240, 0x13},  // character 0x0B: Garuda
                                  {0xb60, 0x13},  // character 0x0C: Ryu
                                  {0xb60, 0x13},  // character 0x0D: Ken
                                  {0xa1c, 0x17},  // character 0x0E: Chun-li
                                  {0x240, 0x13},  // character 0x0F: Zangief
                                  {0x00, 0x13},  // character 0x10: Akuma
                                  {0x00, 0x13},  // character 0x11: Akuma
                                  {0xb60, 0x13},  // character 0x12: Evil Ryu
                                  {0x10c, 0x16},  // character 0x13: Evil Hokuto
                                  {0x904, 0x13},  // character 0x14: M.Bison
                                  {0x240, 0x13},  // character 0x15: Garuda
                                  {0xb60, 0x13},  // character 0x16: Cycloid A
                                  {0xb60, 0x13},  // character 0x17: Cycloid B
                                  {0xb60, 0x13},  // character 0x18: Dhalsim
                                  {0x5b8, 0x13}};  // character 0x19: Sakura

int fps = 33;

int animation_index = 0;
int animations_count;
int change_animation = false;

// settings
const unsigned int SCR_WIDTH = 640;
const unsigned int SCR_HEIGHT = 480;

//FreeFlyCamera * camera;
Camera camera(glm::vec3(0.0f, -3.0, 22.0f));
//Camera camera(glm::vec3(0.0f, 0.0f, 0.1f)); // to view texture

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;
float currentFrame = 0.0f;

bool main_loop = true;
SDL_Event event;
Uint8* keys;

GLuint texture;

uint32_t texture_height;
Texcoord model_min_texcoord[16], model_max_texcoord[16];

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WM_SetCaption("DigiViewer",NULL);
    SDL_SetVideoMode(640, 480, 32, SDL_OPENGL);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    char* pac_file = "DATA/PL/P0C.PAC";

    if(argc > 1)
        pac_file = argv[1];
    /*else
    {
        printf("help: \n\n");
        printf("digiviewer.exe file_name \n\n");
        return 0;
    }*/

    Model* model = load_model(pac_file);
    if(!model)
        return 0;
    animations_count = model->animations_count;
    change_model_animation(model, 45);
    animation_index = 45;
    //print_axis_shifts(model, 45);

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Enable Vertex Arrays
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    //SDL_EnableKeyRepeat(400, 30);

    while (main_loop)
    {
        /*if(SDL_PollEvent(&event) == 1)
        {
            switch(event.type)
            {
                case SDL_QUIT:
                    main_loop = false;
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym)
                    {
                            case SDLK_ESCAPE:
                                main_loop = false;
                                break;
                            default :
                                camera->OnKeyboard(event.key);
                    }
                    break;
                case SDL_KEYUP:
                    camera->OnKeyboard(event.key);
                    break;*/
                /*case SDL_MOUSEMOTION:
                    camera->OnMouseMotion(event.motion);
                    break;*/
                /*case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEBUTTONDOWN:
                    camera->OnMouseButton(event.button);
                    break;
            }
        }*/

        // per-frame time logic
		// --------------------
		currentFrame = SDL_GetTicks();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

        processInput();

        if(change_animation)
        {
            change_model_animation(model, animation_index);
            change_animation = false;
        }

        update_model_animation(model);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // pass projection matrix (note that in this case it could change every frame)
        glm::mat4 projection_mat = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glMatrixMode(GL_PROJECTION); //glLoadIdentity();
        glLoadMatrixf(&projection_mat[0][0]);

        // camera/view transformation
        glm::mat4 view_mat = camera.GetViewMatrix();
        glMatrixMode(GL_MODELVIEW);
        //glLoadMatrixf(&view_mat[0][0]);

        // calculate the model matrix for each object and pass it to shader before drawing
        glm::mat4 model_mat = model_transform(); // make sure to initialize matrix to identity matrix first
        //glMultMatrixf(&model_mat[0][0]);

        glm::mat4 model_view_matrix = view_mat * model_mat;
        glLoadMatrixf(&model_view_matrix[0][0]);

        // render model
        draw_model(model, model_view_matrix);

        sleep();

        glFlush();
        SDL_GL_SwapBuffers();
    }

    //delete camera;
    free_model(model);
    SDL_Quit();

    return 0;
}

void draw_cube(void)
{
    glBegin(GL_QUADS);

    glColor3ub(255,0,0);
    glVertex3d(1,1,1);
    glVertex3d(1,1,-1);
    glVertex3d(-1,1,-1);
    glVertex3d(-1,1,1);

    glColor3ub(0,255,0);
    glVertex3d(1,-1,1);
    glVertex3d(1,-1,-1);
    glVertex3d(1,1,-1);
    glVertex3d(1,1,1);

    glColor3ub(0,0,255);
    glVertex3d(-1,-1,1);
    glVertex3d(-1,-1,-1);
    glVertex3d(1,-1,-1);
    glVertex3d(1,-1,1);

    glColor3ub(255,255,0);
    glVertex3d(-1,1,1);
    glVertex3d(-1,1,-1);
    glVertex3d(-1,-1,-1);
    glVertex3d(-1,-1,1);

    glColor3ub(0,255,255);
    glVertex3d(1,1,-1);
    glVertex3d(1,-1,-1);
    glVertex3d(-1,-1,-1);
    glVertex3d(-1,1,-1);

    glColor3ub(255,0,255);
    glVertex3d(1,-1,1);
    glVertex3d(1,1,1);
    glVertex3d(-1,1,1);
    glVertex3d(-1,-1,1);

    glEnd();
}

void draw_cube2(void)
{
    double v[2][4][3] = {1,1,1, 1,1,-1, -1,1,-1, -1,1,1,
                      1,-1,1, 1,-1,-1, 1,1,-1, 1,1,1};

    glColor3ub(0,255,0);

    int i;

    glBegin(GL_QUADS);

        for(i=0; i < 2; i++)
        {
           glVertex3d(v[i][0][0], v[i][0][1], v[i][0][2]);
           glVertex3d(v[i][1][0], v[i][1][1], v[i][1][2]);
           glVertex3d(v[i][2][0], v[i][2][1], v[i][2][2]);
           glVertex3d(v[i][3][0], v[i][3][1], v[i][3][2]);
        }

    glEnd();
}

void draw_texture(void)
{
    glBindTexture(GL_TEXTURE_2D, texture);

    glBegin(GL_QUADS);

    glColor3ub(255,255,255);
    glTexCoord2d(0, 0); glVertex3d(1,-1,1);
    glTexCoord2d(1, 0);glVertex3d(1,-1,-1);
    glTexCoord2d(1, 1);glVertex3d(1,1,-1);
    glTexCoord2d(0, 1);glVertex3d(1,1,1);

    glEnd();
}

void quad_test(void)
{
    float quad[4][3] = {{1,-1,1}, {1,-1,-1}, {1,1,-1}, {1,1,1}};
    float texcoords[4][2] = {{0,0}, {1,0}, {1,1}, {0,1}};

    //glDisable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, texture);

    /*glBegin(GL_QUADS);

    glColor3ub(0,255,0);
    glTexCoord2d(0, 0);glVertex3fv(&quad[0][0]);
    glTexCoord2d(1, 0);glVertex3fv(&quad[1][0]);
    glTexCoord2d(1, 1);glVertex3fv(&quad[2][0]);
    glTexCoord2d(0, 1);glVertex3fv(&quad[3][0]);

    glEnd();*/

    glVertexPointer(3, GL_FLOAT, 3*sizeof(GLfloat), (GLfloat*)quad);
    glTexCoordPointer(2, GL_FLOAT, 2*sizeof(GLfloat), (GLfloat*)texcoords);
    glDrawArrays(GL_QUADS, 0, 4);

    //glEnable(GL_TEXTURE_2D);
}

void tri_normal_test(FloatTriangle* triangle)
{
    float* v;

    v = &triangle->v1.x;
    glm::vec3 vertex_1(v[0], v[1], v[2]);
    v = &triangle->v2.x;
    glm::vec3 vertex_2(v[0], v[1], v[2]);
    v = &triangle->v3.x;
    glm::vec3 vertex_3(v[0], v[1], v[2]);

    glm::vec3 normal[3], vector_1, vector_2;


    vector_1 = vertex_3 - vertex_1;
    vector_2 = vertex_2 - vertex_1;
    normal[0] = glm::normalize(glm::cross(vector_1, vector_2));
    normal[0] = vertex_1 + (normal[0] * 50.0f);

    vector_1 = vertex_1 - vertex_2;
    vector_2 = vertex_3 - vertex_2;
    normal[1] = glm::normalize(glm::cross(vector_1, vector_2));
    normal[1] = vertex_2 + (normal[1] * 50.0f);

    vector_1 = vertex_2 - vertex_3;
    vector_2 = vertex_1 - vertex_3;
    normal[2] = glm::normalize(glm::cross(vector_1, vector_2));
    normal[2] = vertex_3 + (normal[2] * 50.0f);

    glDisable(GL_TEXTURE_2D);

    /*glBegin(GL_TRIANGLES);
    glColor3ub(0,255,0);
    glVertex3fv(&triangle->v1.x);
    glVertex3fv(&triangle->v2.x);
    glVertex3fv(&triangle->v3.x);
    glEnd();*/

    glBegin(GL_LINES);
    glColor3ub(255,0,0);
    glVertex3fv(&triangle->v1.x);
    glVertex3fv(&normal[0][0]);
    glVertex3fv(&triangle->v2.x);
    glVertex3fv(&normal[1][0]);
    glVertex3fv(&triangle->v3.x);
    glVertex3fv(&normal[2][0]);
    glColor3ub(255,255,255);
    glEnd();

    glEnable(GL_TEXTURE_2D);
}

void quad_normal_test(FloatQuad* quad)
{
    float* v;

    v = &quad->v1.x;
    glm::vec3 vertex_1(v[0], v[1], v[2]);
    v = &quad->v2.x;
    glm::vec3 vertex_2(v[0], v[1], v[2]);
    v = &quad->v3.x;
    glm::vec3 vertex_3(v[0], v[1], v[2]);
    v = &quad->v4.x;
    glm::vec3 vertex_4(v[0], v[1], v[2]);

    glm::vec3 normal[4], vector_1, vector_2;


    vector_1 = vertex_4 - vertex_1;
    vector_2 = vertex_2 - vertex_1;
    normal[0] = glm::normalize(glm::cross(vector_1, vector_2));
    normal[0] = vertex_1 + (normal[0] * 50.0f);

    vector_1 = vertex_1 - vertex_2;
    vector_2 = vertex_3 - vertex_2;
    normal[1] = glm::normalize(glm::cross(vector_1, vector_2));
    normal[1] = vertex_2 + (normal[1] * 50.0f);

    vector_1 = vertex_2 - vertex_3;
    vector_2 = vertex_4 - vertex_3;
    normal[2] = glm::normalize(glm::cross(vector_1, vector_2));
    normal[2] = vertex_3 + (normal[2] * 50.0f);

    vector_1 = vertex_3 - vertex_4;
    vector_2 = vertex_1 - vertex_4;
    normal[3] = glm::normalize(glm::cross(vector_1, vector_2));
    normal[3] = vertex_4 + (normal[3] * 50.0f);

    glDisable(GL_TEXTURE_2D);

    /*glBegin(GL_QUADS);
    glColor3ub(0,255,0);
    glVertex3fv(&quad->v1.x);
    glVertex3fv(&quad->v2.x);
    glVertex3fv(&quad->v3.x);
    glVertex3fv(&quad->v4.x);
    glEnd();*/

    glBegin(GL_LINES);
    glColor3ub(255,0,0);
    glVertex3fv(&quad->v1.x);
    glVertex3fv(&normal[0][0]);
    glVertex3fv(&quad->v2.x);
    glVertex3fv(&normal[1][0]);
    glVertex3fv(&quad->v3.x);
    glVertex3fv(&normal[2][0]);
    glVertex3fv(&quad->v4.x);
    glVertex3fv(&normal[3][0]);
    glColor3ub(255,255,255);
    glEnd();

    glEnable(GL_TEXTURE_2D);
}

void test(void)
{

    /*glBindTexture(GL_TEXTURE_2D, texture);
    glBegin(GL_QUADS);
    glTexCoord2i(0,1);  glVertex3i(-10,-10,-1);
    glTexCoord2d(0,0);  glVertex3i(10,-10,-1);
    glTexCoord2i(1,0);  glVertex3i(10,10,-1);
    glTexCoord2i(1,1);  glVertex3i(-10,10,-1);*/

    glBindTexture(GL_TEXTURE_2D, texture);
    glBegin(GL_TRIANGLES);
    glTexCoord2d(0.666666, 0.607843);  glVertex3i(204, 0, -86);
    glTexCoord2d(0.015686, 0.576470);  glVertex3i(113, -20, 0);
    glTexCoord2d(0.533333, 0.247058);  glVertex3i(-21, -256, -86);

    glEnd();
}

void processInput(void)
{
    if(SDL_PollEvent(&event) == 1)
    {
        switch(event.type)
        {
            case SDL_QUIT:
                main_loop = false;
                break;
            /*case SDL_VIDEORESIZE:
                SDL_SetVideoMode(event.resize.w, event.resize.h, 32, SDL_OPENGL|SDL_RESIZABLE);
                glViewport(0, 0, event.resize.w, event.resize.h);
                break;*/
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        main_loop = 0;
                        break;
                    case SDLK_p:
                        animation_index += 1;
                        if(animation_index > animations_count-1)
                            animation_index = 0;
                        change_animation = true;
                        break;
                    case SDLK_o:
                        animation_index -= 1;
                        if(animation_index < 0)
                            animation_index = animations_count-1;
                        change_animation = true;
                        break;
                }
                break;
            case SDL_MOUSEMOTION:
            {
                float xpos = static_cast<float>(event.motion.x);
                float ypos = static_cast<float>(event.motion.y);

                if (firstMouse)
                {
                    lastX = xpos;
                    lastY = ypos;
                    firstMouse = false;
                }

                float xoffset = xpos - lastX;
                float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

                lastX = xpos;
                lastY = ypos;

                camera.ProcessMouseMovement(xoffset, yoffset);
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            {
                if (event.button.button == SDL_BUTTON_WHEELUP)
                {
                    camera.ProcessMouseScroll(static_cast<float>(2.0f));
                }
                else if (event.button.button == SDL_BUTTON_WHEELDOWN)
                {
                    camera.ProcessMouseScroll(static_cast<float>(-2.0f));
                }
                break;
            }

        }
    }

    keys = SDL_GetKeyState(NULL);

    if(keys[SDLK_ESCAPE])
        main_loop = 0;

    if(keys[SDLK_w])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    else if(keys[SDLK_a])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if(keys[SDLK_s])
        camera.ProcessKeyboard(LEFT, deltaTime);
    else if(keys[SDLK_d])
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if(keys[SDLK_UP])
        camera.ProcessMouseMovement(0, 10);
    else if(keys[SDLK_DOWN])
        camera.ProcessMouseMovement(0, -10);
    if(keys[SDLK_LEFT])
        camera.ProcessMouseMovement(-10, 0);
    else if(keys[SDLK_RIGHT])
        camera.ProcessMouseMovement(10, 0);
}

void sleep(void)
{
    static int old_time = 0,  actual_time = 0;
    actual_time = SDL_GetTicks();
    if (actual_time - old_time < fps) // if less than fps ms has passed
    {
        SDL_Delay(fps - (actual_time - old_time));
        old_time = SDL_GetTicks();
    }
    else
    {
        old_time = actual_time;
    }
}

GLuint loadTexture(const char * filename,bool useMipMap)
{
    GLuint glID;
    SDL_Surface * picture_surface = NULL;
    SDL_Surface *gl_surface = NULL;
    Uint32 rmask, gmask, bmask, amask;

    picture_surface = SDL_LoadBMP(filename);
    if (picture_surface == NULL)
        return 0;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else

    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    SDL_PixelFormat format = *(picture_surface->format);
    format.BitsPerPixel = 32;
    format.BytesPerPixel = 4;
    format.Rmask = rmask;
    format.Gmask = gmask;
    format.Bmask = bmask;
    format.Amask = amask;

    gl_surface = SDL_ConvertSurface(picture_surface,&format,SDL_SWSURFACE);

    glGenTextures(1, &glID);

    glBindTexture(GL_TEXTURE_2D, glID);


    if (useMipMap)
    {

        gluBuild2DMipmaps(GL_TEXTURE_2D, 4, gl_surface->w,
                          gl_surface->h, GL_RGBA,GL_UNSIGNED_BYTE,
                          gl_surface->pixels);

        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);

    }
    else
    {
        glTexImage2D(GL_TEXTURE_2D, 0, 4, gl_surface->w,
                     gl_surface->h, 0, GL_RGBA,GL_UNSIGNED_BYTE,
                     gl_surface->pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);


    SDL_FreeSurface(gl_surface);
    SDL_FreeSurface(picture_surface);

    return glID;
}

int get_file_size(FILE* file)
{
    fseek (file, 0, SEEK_END);
    int file_size = ftell(file);
    fseek (file, 0, SEEK_SET);
    return file_size;
}


void load_4bit_tim_pixels(Tim* tim, uint32_t clut_number, uint16_t *pixels_data)
{
    uint16_t clut_index[4], color;
    uint8_t r5, g5, b5, a1;
    uint32_t pixel_index = 0;
    clut_number %= tim->cluts_count;
    uint16_t* clut_ptr = tim->clut + (16 * clut_number);
    uint8_t* image_pixels = tim->image_pixels + (tim->image_real_width * 4 * tim->image_height * clut_number);
    for(int i = 0; i < (tim->image_width * tim->image_height); i++)
    {
        clut_index[0] = pixels_data[i] & 0xF;
        clut_index[1] = (pixels_data[i] >> 4) & 0xF;
        clut_index[2] = (pixels_data[i] >> 8) & 0xF;
        clut_index[3] = (pixels_data[i] >> 12) & 0xF;

        for(int j = 0; j < 4; j++)
        {
            color = clut_ptr[clut_index[j]];
            r5 = color & 0x1F;
            g5 = (color >> 5) & 0x1F;
            b5 = (color >> 10) & 0x1F;
            a1 = (color >> 15) & 0x1F;

            /*image_pixels[pixel_index++] = r5 * (255/31); // R
            image_pixels[pixel_index++] = g5 * (255/31); // G
            image_pixels[pixel_index++] = b5 * (255/31); // B*/
            // Bit-Walking / Shifting instead of division or multiplication
            // (val << 3) | (val >> 2) scales 5 bits to 8 bits flawlessly (0 -> 0, 31 -> 255)
            image_pixels[pixel_index++] = (r5 << 3) | (r5 >> 2); // R
            image_pixels[pixel_index++] = (g5 << 3) | (g5 >> 2); // G
            image_pixels[pixel_index++] = (b5 << 3) | (b5 >> 2); // B
            image_pixels[pixel_index++] = a1 ? 255 : 0;  // A
        }
    }
}

GLuint load_tim(char* data, bool useMipMap, uint32_t max_clut_index)
{
    /*FILE* file = fopen(file_path, "rb");

    if(!file)
    {
        printf("couldn't open file: %s \n", file_path);
        return 0;
    }

    unsigned int data_size = get_file_size(file);
    char* data = (char*)malloc(data_size);
    fread(data, data_size, 1, file);
    fclose(file);*/

    char* file_path = "pac_texture_data";

    uint32_t* data_ptr = (uint32_t*)data;
    uint16_t* data_ptr_2;
    uint16_t clut_index[4], color;
    uint32_t pixel_index;
    uint8_t r5, g5, b5, a1;

    GLuint glID;
    Tim* tim;

    if(data_ptr[0] != 0x10)
    {
        printf("file: %s is not a tim \n", file_path);
        return 0;
    }

    tim = (Tim*)malloc(sizeof(Tim));
    memset(tim, 0, sizeof(Tim));

    printf("\n ... parsing tim file: %s ... \n", file_path);

    switch(data_ptr[1] & 7)
    {
        case 0:
            printf("tim has %d bit \n", 4);
            tim->pixel_mode = 4;
            break;
        case 1:
            printf("tim has %d bit \n", 8);
            tim->pixel_mode = 8;
            break;
        case 2:
            printf("tim has %d bit \n", 16);
            tim->pixel_mode = 16;
            break;
        case 3:
            printf("tim has %d bit \n", 24);
            tim->pixel_mode = 24;
            break;
        case 4: printf("tim has mixed bit \n"); break;
    }

    tim->has_clut = data_ptr[1] & 8;

    if(tim->has_clut)
    {
        tim->clut_x = data_ptr[3] & 0xFFFF;
        tim->clut_y = data_ptr[3] >> 16;
        tim->clut_width = data_ptr[4] & 0xFFFF;
        tim->clut_height = data_ptr[4] >> 16;
        tim->clut = (uint16_t*)malloc((tim->clut_width * tim->clut_height) * sizeof(short));
        data_ptr_2 = (uint16_t*)(&data_ptr[5]);
        for(int i = 0; i < (tim->clut_width * tim->clut_height); i++)
			tim->clut[i] = data_ptr_2[i];

        printf("tim has clut \n");
        printf("clut block size: %d --> %d \n", data_ptr[2], (data_ptr[2]-12)/2);
        printf("clut info: x=%d y=%d w=%d h=%d \n",
               tim->clut_x, tim->clut_y, tim->clut_width, tim->clut_height);

        switch(tim->pixel_mode)
        {
            case 4:
                tim->cluts_count = tim->clut_width / 16;
                printf("cluts count: %d \n", tim->cluts_count);
                break;
            case 8:
                tim->cluts_count = tim->clut_width / 256;
                printf("cluts count: %d \n", tim->cluts_count);
                break;
        }
    }
    else
    {
        printf("tim doesn't have clut \n");
    }

    data_ptr = (uint32_t*)(data + 8 + data_ptr[2]);
    printf("pixels data block size: %d \n", data_ptr[0]);

    tim->image_x = data_ptr[1] & 0xFFFF;
    tim->image_y  = data_ptr[1] >> 16;
    tim->image_width = data_ptr[2] & 0xFFFF;
    tim->image_height  = data_ptr[2] >> 16;

    switch(tim->pixel_mode)
    {
        case 4: tim->image_real_width = tim->image_width * 4; break;
        case 8: tim->image_real_width = tim->image_width * 2; break;
    }

    printf("image pixels info: x=%d y=%d w=%d h=%d \n",
               tim->image_x, tim->image_y,
               tim->image_width, tim->image_height);

    tim->image_pixels = (uint8_t*)malloc((tim->image_real_width * 4 * tim->image_height) * max_clut_index * sizeof(uint8_t));
    data_ptr_2 = (uint16_t*)(&data_ptr[3]);
    if(tim->pixel_mode == 4)
    {
        for(int i = 0; i < max_clut_index; i++)
            load_4bit_tim_pixels(tim, i, data_ptr_2);
    }

    glGenTextures(1, &glID);

    glBindTexture(GL_TEXTURE_2D, glID);


    if (useMipMap)
    {

        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, tim->image_width * 4,
                          tim->image_height * max_clut_index, GL_RGBA,GL_UNSIGNED_BYTE,
                          tim->image_pixels);

        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);

    }
    else
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tim->image_width * 4,
                     tim->image_height * max_clut_index, 0, GL_RGBA,GL_UNSIGNED_BYTE,
                     tim->image_pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    //free(data);
    free(tim->clut);
    free(tim->image_pixels);
    free(tim);

    return glID;
}

Pac* load_pac(char* file_path)
{
    FILE* file = fopen(file_path, "rb");

    if(!file)
    {
        printf("couldn't open file: %s \n", file_path);
        return 0;
    }

    Pac* pac = (Pac*)malloc(sizeof(Pac));
    memset(pac, 0, sizeof(Pac));
    unsigned int data_size = get_file_size(file);
    pac->data = (char*)malloc(data_size);
    fread(pac->data, data_size, 1, file);
    fclose(file);

    uint32_t* data_offset = (uint32_t*)pac->data;
    pac->animations_data = &pac->data[data_offset[0]];
    pac->meshes_data = &pac->data[data_offset[1]];
    pac->texture_data =  &pac->data[data_offset[3]];

    pac->animations_data_size = data_offset[1] - data_offset[0];
    pac->meshes_data_size = data_offset[2] - data_offset[1];
    pac->texture_data_size = data_offset[4] - data_offset[3];

    return pac;
}

void free_pac(Pac* pac)
{
    free(pac->data); pac->data = NULL;
    free(pac); pac = NULL;
}

size_t get_primtive_count(char* data, size_t data_size)
{
    int32_t* data_ptr = (int*)data;
    int32_t delimiter = data_ptr[1];
    size_t primtive_count = 0;
    for(size_t i = 0; i < data_size / 4; i++)
    {
        if (data_ptr[i] == delimiter)
            primtive_count++;
    }
    return primtive_count;
}

Mesh* read_mesh_tri(char* file_path)
{
    FILE* file = fopen(file_path, "rb");

    if(!file)
        printf("couldn't open file: %s \n", file_path);

    unsigned int data_size = get_file_size(file);
    char* data = (char*)malloc(data_size);
    fread(data, data_size, 1, file);
    fclose(file);
    int32_t delimiter = ((int32_t*)data)[1];

    size_t triangles_count = get_primtive_count(data, data_size);
    Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));
    mesh->triangles_count = triangles_count;
    mesh->triangles = (Triangle*)malloc(sizeof(Triangle)*triangles_count);
    int index = 0;

    int32_t* data_ptr = (int32_t*)data;
    for(size_t i = 0; i < data_size / 4; i++)
    {
        if (data_ptr[i] == delimiter)
        {
             mesh->triangles[index] = *((Triangle*)(&data_ptr[i+1]));
             index++;
        }
    }

    free(data);
    return mesh;
}

Mesh* read_mesh_quad(char* file_path)
{
    FILE* file = fopen(file_path, "rb");

    if(!file)
        printf("couldn't open file: %s \n", file_path);

    unsigned int data_size = get_file_size(file);
    char* data = (char*)malloc(data_size);
    fread(data, data_size, 1, file);
    fclose(file);
    int32_t delimiter = ((int32_t*)data)[1];

    size_t quads_count = get_primtive_count(data, data_size);
    Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));
    mesh->quads_count = quads_count;
    mesh->quads = (Quad*)malloc(sizeof(Quad)*quads_count);
    int index = 0;

    int32_t* data_ptr = (int32_t*)data;
    for(size_t i = 0; i < data_size / 4; i++)
    {
        if (data_ptr[i] == delimiter)
        {
             mesh->quads[index] = *((Quad*)(&data_ptr[i+1]));
             index++;
        }
    }

    free(data);
    return mesh;
}

uint32_t get_primtive_count_2(char* data, size_t data_size)
{
    char* data_ptr = data;
    uint32_t triangles_count = 0;
    uint32_t quads_count = 0;
    uint16_t obj_count;
    uint16_t obj_type;
    uint32_t stride = 0;

    while(data_ptr < data + data_size)
    {

        obj_count = ((uint16_t*)data_ptr)[0];
        obj_type = ((uint16_t*)data_ptr)[1];

        switch(obj_type)
        {
            case 0x24:
                stride = 44;
                triangles_count += obj_count;
                break;
            case 0x0c:
            case 0x34:
                stride = 60;
                triangles_count += obj_count;
                break;
            case 0x2c:
                stride = 52;
                quads_count += obj_count;
                break;
            case 0x3c:
                stride = 76;
                quads_count += obj_count;
                break;
            default:
                printf("unsupported primitive type 0x%2x ! \n", obj_type);
                break;
        }

        data_ptr += (stride * obj_count) + 4;

    }

    uint32_t primtive_count = triangles_count | (quads_count << 16);

    return primtive_count;
}

Mesh* read_mesh_2(char* file_path)
{
    FILE* file = fopen(file_path, "rb");

    if(!file)
        printf("couldn't open file: %s \n", file_path);

    unsigned int data_size = get_file_size(file);
    char* data = (char*)malloc(data_size);
    fread(data, data_size, 1, file);
    fclose(file);
    int32_t delimiter = ((int32_t*)data)[1];

    uint32_t primtive_count = get_primtive_count_2(data, data_size);
    Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));
    memset(mesh, 0, sizeof(Mesh));
    mesh->triangles_count = primtive_count & 0x0000ffff;
    if(mesh->triangles_count > 0)
        mesh->triangles = (Triangle*)malloc(sizeof(Triangle)*mesh->triangles_count);
    mesh->quads_count = primtive_count >> 16;
    if(mesh->quads_count > 0)
        mesh->quads = (Quad*)malloc(sizeof(Quad)*mesh->quads_count);

    char* data_ptr = data;
    uint16_t obj_count;
    uint16_t obj_type;
    uint32_t stride = 0;
    unsigned int tri_index = 0, quad_index = 0, i;

    while(data_ptr < data + data_size)
    {

        obj_count = ((uint16_t*)data_ptr)[0];
        obj_type = ((uint16_t*)data_ptr)[1];

        switch(obj_type)
        {
            case 0x24:
                stride = 44;
                break;
            case 0x0c:
            case 0x34:
                stride = 60;
                break;
            case 0x2c:
                stride = 52;
                break;
            case 0x3c:
                stride = 76;
                break;
        }

        if(obj_type == 0x24 || obj_type == 0x0c || obj_type == 0x34)
        {
            for(i = 0; i < obj_count; i++)
            {
                mesh->triangles[tri_index] = *((Triangle*)(&data_ptr[i*stride+8]));
                tri_index++;
            }
        }
        else if(obj_type == 0x2c || obj_type == 0x3c)
        {
            for(i = 0; i < obj_count; i++)
            {
                mesh->quads[quad_index] = *((Quad*)(&data_ptr[i*stride+8]));
                quad_index++;
            }
        }

        data_ptr += (stride * obj_count) + 4;

    }

    free(data);
    return mesh;
}

size_t get_primtive_count_3(char* data, size_t data_size)
{
    char* data_ptr = data;
    uint32_t delimiter = ((uint32_t*)data_ptr)[1];
    uint32_t triangles_count = 0;
    uint32_t quads_count = 0;
    uint16_t obj_type;
    uint32_t stride = 0;
    uint32_t* primtive_ptr, i = 0;

    while(data_ptr < data + data_size)
    {

        delimiter = ((uint32_t*)data_ptr)[1];
        obj_type = ((uint16_t*)data_ptr)[1];
        //printf("obj_type: 0x%x  ptr: 0x%x \n", obj_type, data_ptr - data);;

        switch(obj_type)
        {
            case 0x24:
                stride = 44;
                primtive_ptr = &triangles_count;
                break;
            case 0x0c:
            case 0x34:
                stride = 60;
                primtive_ptr = &triangles_count;
                break;
            case 0x2c:
                stride = 52;
                primtive_ptr = &quads_count;
                break;
            case 0x3c:
                stride = 76;
                primtive_ptr = &quads_count;
                break;
            default:
                printf("unsupported primitive type 0x%x ptr:0x%x ! \n", obj_type, data_ptr - data);
                break;
        }

        data_ptr += 4;i=0;

        while((delimiter & 0x0000ffff) == 0x0028 && (delimiter >> 24) == 0x78)
        {
            data_ptr += stride;
            (*primtive_ptr)++;i++;
            delimiter = ((uint32_t*)data_ptr)[0];
        }
        //printf("i=%d obj_type=0x%x \n", i, obj_type);

    }

    uint32_t primtive_count = triangles_count | (quads_count << 16);
    //printf("tri=%d quads=%d \n", triangles_count, quads_count);
    return primtive_count;
}

Mesh* read_mesh_3(char* file_path)
{
    FILE* file = fopen(file_path, "rb");

    if(!file)
        printf("couldn't open file: %s \n", file_path);

    unsigned int data_size = get_file_size(file);
    char* data = (char*)malloc(data_size);
    fread(data, data_size, 1, file);
    fclose(file);

    uint32_t primtive_count = get_primtive_count_3(data, data_size);

    Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));
    memset(mesh, 0, sizeof(Mesh));
    mesh->triangles_count = primtive_count & 0x0000ffff;
    if(mesh->triangles_count > 0)
        mesh->triangles = (Triangle*)malloc(sizeof(Triangle)*mesh->triangles_count);
    mesh->quads_count = primtive_count >> 16;
    if(mesh->quads_count > 0)
        mesh->quads = (Quad*)malloc(sizeof(Quad)*mesh->quads_count);

    char* data_ptr = data;
    uint32_t delimiter = ((uint32_t*)data)[1];
    uint16_t obj_type;
    uint32_t stride = 0;
    unsigned int tri_index = 0, quad_index = 0;

    while(data_ptr < data + data_size)
    {

        delimiter = ((uint32_t*)data_ptr)[1];
        obj_type = ((uint16_t*)data_ptr)[1];

        switch(obj_type)
        {
            case 0x24:
                stride = 44;
                break;
            case 0x0c:
            case 0x34:
                stride = 60;
                break;
            case 0x2c:
                stride = 52;
                break;
            case 0x3c:
                stride = 76;
                break;
        }

        data_ptr += 4;

        while((delimiter & 0x0000ffff) == 0x0028 && (delimiter >> 24) == 0x78)
        {
            if(obj_type == 0x24 || obj_type == 0x0c || obj_type == 0x34)
            {
                mesh->triangles[tri_index] = *((Triangle*) (data_ptr+4));
                tri_index++;
            }
            else if(obj_type == 0x2c || obj_type == 0x3c)
            {
                mesh->quads[quad_index] = *((Quad*) (data_ptr+4));
                quad_index++;
            }
            data_ptr += stride;
            delimiter = ((uint32_t*)data_ptr)[0];
        }

    }

    free(data);
    return mesh;
}

#define max(a,b)	((a) > (b) ? (a) : (b))
#define min(a,b)	((a) < (b) ? (a) : (b))

void get_min_texcoord(Texcoord* t1, Texcoord* t2, Texcoord* t3, Texcoord* t4, Texcoord* result)
{
    result->x = min(min(t1->x, t2->x), min(t3->x, t4->x));
    result->y = min(min(t1->y, t2->y), min(t3->y, t4->y));
}

void get_max_texcoord(Texcoord* t1, Texcoord* t2, Texcoord* t3, Texcoord* t4, Texcoord* result)
{
    result->x = max(max(t1->x, t2->x), max(t3->x, t4->x));
    result->y = max(max(t1->y, t2->y), max(t3->y, t4->y));
}

Mesh* read_mesh_4(char* file_path)
{
    FILE* file = fopen(file_path, "rb");

    if(!file)
        printf("couldn't open file: %s \n", file_path);

    printf("\n ... reading mesh: %s ... \n", file_path);
    printf("printing delimiters ... \n");

    unsigned int data_size = get_file_size(file);
    char* data = (char*)malloc(data_size);
    fread(data, data_size, 1, file);
    fclose(file);

    uint32_t primtive_count = get_primtive_count_3(data, data_size);

    Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));
    memset(mesh, 0, sizeof(Mesh));
    mesh->triangles_count = primtive_count & 0x0000ffff;
    if(mesh->triangles_count > 0)
        mesh->triangles = (Triangle*)malloc(sizeof(Triangle)*mesh->triangles_count);
    mesh->quads_count = primtive_count >> 16;
    if(mesh->quads_count > 0)
        mesh->quads = (Quad*)malloc(sizeof(Quad)*mesh->quads_count);

    char* data_ptr = data;
    uint32_t delimiter = ((uint32_t*)data)[1];
    uint32_t old_delimiter = delimiter;
    uint32_t clut_index = 0;
    uint16_t obj_type;
    uint32_t stride = 0;
    unsigned int tri_index = 0, quad_index = 0;
    Triangle temp_tri; Quad temp_quad;
    Texcoord min_texcoord[16], max_texcoord[16], temp_texcoord;
    memset(min_texcoord, 0xff, sizeof(Texcoord) * 16);
    memset(max_texcoord, 0, sizeof(Texcoord) * 16);

    while(data_ptr < data + data_size)
    {

        delimiter = ((uint32_t*)data_ptr)[1];
        obj_type = ((uint16_t*)data_ptr)[1];

        switch(obj_type)
        {
            case 0x24:
                stride = 44;
                break;
            case 0x0c:
            case 0x34:
                stride = 60;
                break;
            case 0x2c:
                stride = 52;
                break;
            case 0x3c:
                stride = 76;
                break;
        }

        data_ptr += 4;

        while((delimiter & 0x0000ffff) == 0x0028 && (delimiter >> 24) == 0x78)
        {
            clut_index = (delimiter >> 16) & 0xff;

            if(obj_type == 0x24 || obj_type == 0x0c || obj_type == 0x34)
            {
                mesh->triangles[tri_index] = *((Triangle*) (data_ptr+4));

                temp_tri = mesh->triangles[tri_index];
                get_min_texcoord(&temp_tri.t1, &temp_tri.t2, &temp_tri.t3, &temp_tri.t1, &temp_texcoord);
                min_texcoord[clut_index].x = min(temp_texcoord.x, min_texcoord[clut_index].x);
                min_texcoord[clut_index].y = min(temp_texcoord.y, min_texcoord[clut_index].y);
                get_max_texcoord(&temp_tri.t1, &temp_tri.t2, &temp_tri.t3, &temp_tri.t1, &temp_texcoord);
                max_texcoord[clut_index].x = max(temp_texcoord.x, max_texcoord[clut_index].x);
                max_texcoord[clut_index].y = max(temp_texcoord.y, max_texcoord[clut_index].y);

                tri_index++;
            }
            else if(obj_type == 0x2c || obj_type == 0x3c)
            {
                mesh->quads[quad_index] = *((Quad*) (data_ptr+4));

                temp_quad = mesh->quads[quad_index];
                get_min_texcoord(&temp_tri.t1, &temp_tri.t2, &temp_tri.t3, &temp_tri.t4, &temp_texcoord);
                min_texcoord[clut_index].x = min(temp_texcoord.x, min_texcoord[clut_index].x);
                min_texcoord[clut_index].y = min(temp_texcoord.y, min_texcoord[clut_index].y);
                get_max_texcoord(&temp_tri.t1, &temp_tri.t2, &temp_tri.t3, &temp_tri.t4, &temp_texcoord);
                max_texcoord[clut_index].x = max(temp_texcoord.x, max_texcoord[clut_index].x);
                max_texcoord[clut_index].y = max(temp_texcoord.y, max_texcoord[clut_index].y);

                quad_index++;
            }
            if(delimiter != old_delimiter)
                printf("delimiter changed ! ");
            old_delimiter = delimiter;
            printf("delimiter: 0x%x CLUT: 0x%x \n", delimiter, clut_index);
            data_ptr += stride;
            delimiter = ((uint32_t*)data_ptr)[0];
        }

    }

    printf("\n ... printing min, max texcoords ... \n");
    for(int i = 0; i < 16; i++)
    {
        printf("\n ... CLUT %d ... \n", i);
        if(!(min_texcoord[i].x == 0xff && min_texcoord[i].y == 0xff))
        {
            printf("min texcoord: x=%d  y=%d \n", min_texcoord[i].x, min_texcoord[i].y);
            model_min_texcoord[i].x = min(min_texcoord[i].x, model_min_texcoord[i].x);
            model_min_texcoord[i].y = min(min_texcoord[i].y, model_min_texcoord[i].y);
        }
        if(!(max_texcoord[i].x == 0x00 && max_texcoord[i].y == 0x00))
        {
            printf("max texcoord: x=%d  y=%d \n", max_texcoord[i].x, max_texcoord[i].y);
            model_max_texcoord[i].x = max(max_texcoord[i].x, model_max_texcoord[i].x);
            model_max_texcoord[i].y = max(max_texcoord[i].y, model_max_texcoord[i].y);
        }
    }

    free(data);
    return mesh;
}

void free_mesh_tri(Mesh* mesh)
{
    free(mesh->triangles); mesh->triangles = NULL;
    free(mesh); mesh = NULL;
}

void free_mesh_quad(Mesh* mesh)
{
    free(mesh->quads); mesh->quads = NULL;
    free(mesh); mesh = NULL;
}

void free_mesh_2(Mesh* mesh)
{
    free(mesh->triangles); mesh->triangles = NULL;
    free(mesh->quads); mesh->quads = NULL;
    free(mesh->tri_texcoords); mesh->tri_texcoords = NULL;
    free(mesh->quad_texcoords); mesh->quad_texcoords = NULL;
    free(mesh->float_triangles); mesh->float_triangles = NULL;
    free(mesh->float_quads); mesh->float_quads = NULL;
    free(mesh->triangle_normals); mesh->triangle_normals = NULL;
    free(mesh->quad_normals); mesh->quad_normals = NULL;
    free(mesh); mesh = NULL;
}

void print_mesh_tri(Mesh* mesh)
{
    Triangle *tri;
    printf("\n ... printing mesh data ... \n");
    printf("triangles_count: %d \n", mesh->triangles_count);
    for(int i = 0; i < mesh->triangles_count; i ++)
    {
        tri = &mesh->triangles[i];
        printf("triangle:%d \n", i);
        printf("x1:%d  y1:%d  z1:%d \n", tri->v1.x, tri->v1.y, tri->v1.z);
        printf("x2:%d  y2:%d  z2:%d \n", tri->v2.x, tri->v2.y, tri->v2.z);
        printf("x3:%d  y3:%d  z3:%d \n", tri->v3.x, tri->v3.y, tri->v3.z);
    }
}

void print_mesh_quad(Mesh* mesh)
{

}

void draw_mesh_tri(Mesh* mesh)
{

    Triangle *tri;
    TriTexcoord* texcoord;
    float scale = 1;
    float tex_size_x = 256;//1280;
    float tex_size_y = texture_height;


    glBindTexture(GL_TEXTURE_2D, texture);

    glBegin(GL_TRIANGLES);

    //glColor3ub(255,0,0);

    for(int i = 0; i < mesh->triangles_count; i++)
    {
        tri = &mesh->triangles[i];
        texcoord = &mesh->tri_texcoords[i];

        glTexCoord2d(tri->t1.x/tex_size_x, texcoord->t1.y/tex_size_y);  glVertex3d(tri->v1.x/scale, tri->v1.y/scale, tri->v1.z/scale);
        glTexCoord2d(tri->t2.x/tex_size_x, texcoord->t2.y/tex_size_y);  glVertex3d(tri->v2.x/scale, tri->v2.y/scale, tri->v2.z/scale);
        glTexCoord2d(tri->t3.x/tex_size_x, texcoord->t3.y/tex_size_y);  glVertex3d(tri->v3.x/scale, tri->v3.y/scale, tri->v3.z/scale);
    }

    glEnd();
}

void draw_mesh_quad(Mesh* mesh)
{

    Quad *quad;
    QuadTexcoord* texcoord;
    float scale = 1;
    float tex_size_x = 256;//1280;
    float tex_size_y = texture_height;

    glBindTexture(GL_TEXTURE_2D, texture);

    glBegin(GL_QUADS);

    //glColor3ub(255,0,0);

    for(int i = 0; i < mesh->quads_count; i++)
    {
        quad = &mesh->quads[i];
        texcoord = &mesh->quad_texcoords[i];

        glTexCoord2d(quad->t1.x/tex_size_x, texcoord->t1.y/tex_size_y);  glVertex3d(quad->v1.x/scale, quad->v1.y/scale, quad->v1.z/scale);
        glTexCoord2d(quad->t2.x/tex_size_x, texcoord->t2.y/tex_size_y);  glVertex3d(quad->v2.x/scale, quad->v2.y/scale, quad->v2.z/scale);
        glTexCoord2d(quad->t3.x/tex_size_x, texcoord->t3.y/tex_size_y);  glVertex3d(quad->v3.x/scale, quad->v3.y/scale, quad->v3.z/scale);
        glTexCoord2d(quad->t4.x/tex_size_x, texcoord->t4.y/tex_size_y);  glVertex3d(quad->v4.x/scale, quad->v4.y/scale, quad->v4.z/scale);
    }

    glEnd();
}

void draw_mesh(Mesh* mesh)
{
    draw_mesh_tri(mesh);
    draw_mesh_quad(mesh);
}

glm::mat4 model_transform(void)
{
    static float scale = 1.0f / 32.0f; // 0.03125f
    glm::mat4 model_mat = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
    //model_mat = translate(model_mat, glm::vec3(5.0f, 0.0f, 0.0f));
    model_mat = glm::scale(model_mat, glm::vec3(scale, scale, scale));
    // to view texture
    //model_mat = glm::rotate(model_mat, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    //model_mat = glm::rotate(model_mat, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    return model_mat;
}

glm::mat4 get_bone_matrix(float rot_angles[3], Vec3 *init_pos)
{
    //float scale = 32;
    glm::mat4 model_mat = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
    int16_t x = (int16_t) ((int32_t) (init_pos->x * 0xF) >> 7);
    int16_t y = (int16_t) ((int32_t) (init_pos->y * 0xF) >> 7);
    int16_t z = (int16_t) ((int32_t) (init_pos->z * 0xF) >> 7);

    //glm::vec3 translation_vec(-x/scale, -y/scale, -z/scale);
    glm::vec3 translation_vec(-x, -y, -z);
    model_mat = glm::translate(model_mat, translation_vec);

    // added rotation
    model_mat = glm::rotate(model_mat, glm::radians(rot_angles[0]), glm::vec3(-1.0f, 0.0f, 0.0f));
    model_mat = glm::rotate(model_mat, glm::radians(rot_angles[1]), glm::vec3(0.0f, -1.0f, 0.0f));
    model_mat = glm::rotate(model_mat, glm::radians(rot_angles[2]), glm::vec3(0.0f, 0.0f, -1.0f));

    return model_mat;
}

Mesh* read_mesh_data(char* data, unsigned int data_size, uint8_t* used_cluts)
{
    uint32_t primtive_count = get_primtive_count_3(data, data_size);

    Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));
    memset(mesh, 0, sizeof(Mesh));
    mesh->triangles_count = primtive_count & 0x0000ffff;
    if(mesh->triangles_count > 0)
    {
        mesh->triangles = (Triangle*)malloc(sizeof(Triangle)*mesh->triangles_count);
        mesh->tri_texcoords = (TriTexcoord*)malloc(sizeof(TriTexcoord)*mesh->triangles_count);
        mesh->float_triangles = (FloatTriangle*)malloc(sizeof(FloatTriangle)*mesh->triangles_count);
        mesh->triangle_normals = (FloatTriangle*)malloc(sizeof(FloatTriangle)*mesh->triangles_count);
    }
    mesh->quads_count = primtive_count >> 16;
    if(mesh->quads_count > 0)
    {
        mesh->quads = (Quad*)malloc(sizeof(Quad)*mesh->quads_count);
        mesh->quad_texcoords = (QuadTexcoord*)malloc(sizeof(QuadTexcoord)*mesh->quads_count);
        mesh->float_quads = (FloatQuad*)malloc(sizeof(FloatQuad)*mesh->quads_count);
        mesh->quad_normals = (FloatQuad*)malloc(sizeof(FloatQuad)*mesh->quads_count);
    }

    char* data_ptr = data;
    uint32_t delimiter = ((uint32_t*)data)[1];
    uint32_t clut_index = 0;
    uint32_t texcoord_offset = 0;
    uint16_t obj_type;
    uint32_t stride = 0;
    unsigned int tri_index = 0, quad_index = 0;
    Triangle* tri_ptr; Quad* quad_ptr;
    TriTexcoord* tri_texcoord_ptr;
    QuadTexcoord* quad_texcoord_ptr;

    while(data_ptr < data + data_size)
    {

        delimiter = ((uint32_t*)data_ptr)[1];
        obj_type = ((uint16_t*)data_ptr)[1];

        switch(obj_type)
        {
            case 0x24:
                stride = 44;
                break;
            case 0x0c:
            case 0x34:
                stride = 60;
                break;
            case 0x2c:
                stride = 52;
                break;
            case 0x3c:
                stride = 76;
                break;
        }

        data_ptr += 4;

        while((delimiter & 0x0000ffff) == 0x0028 && (delimiter >> 24) == 0x78)
        {
            clut_index = (delimiter >> 16) & 0xff;
            texcoord_offset = clut_index * 256;
            used_cluts[clut_index] = 1;

            if(obj_type == 0x24 || obj_type == 0x0c || obj_type == 0x34)
            {
                mesh->triangles[tri_index] = *((Triangle*) (data_ptr+4));
                tri_ptr = &mesh->triangles[tri_index];
                tri_texcoord_ptr =  &mesh->tri_texcoords[tri_index];
                tri_index++;
                tri_texcoord_ptr->t1.y = tri_ptr->t1.y + texcoord_offset;
                tri_texcoord_ptr->t2.y = tri_ptr->t2.y + texcoord_offset;
                tri_texcoord_ptr->t3.y = tri_ptr->t3.y + texcoord_offset;
            }
            else if(obj_type == 0x2c || obj_type == 0x3c)
            {
                mesh->quads[quad_index] = *((Quad*) (data_ptr+4));
                quad_ptr = &mesh->quads[quad_index];
                quad_texcoord_ptr =  &mesh->quad_texcoords[quad_index];
                quad_index++;
                quad_texcoord_ptr->t1.y = quad_ptr->t1.y + texcoord_offset;
                quad_texcoord_ptr->t2.y = quad_ptr->t2.y + texcoord_offset;
                quad_texcoord_ptr->t3.y = quad_ptr->t3.y + texcoord_offset;
                quad_texcoord_ptr->t4.y = quad_ptr->t4.y + texcoord_offset;
            }
            data_ptr += stride;
            delimiter = ((uint32_t*)data_ptr)[0];
        }

    }

    return mesh;
}

Model* read_model_meshes_2(char* file_path)
{
    FILE* file = fopen(file_path, "rb");

    if(!file)
        printf("couldn't open file: %s \n", file_path);

    unsigned int data_size = get_file_size(file);
    char* data = (char*)malloc(data_size);
    fread(data, data_size, 1, file);
    fclose(file);

    uint32_t meshes_count = ((uint32_t*)data)[0];

    Model* model = (Model*)malloc(sizeof(Model));
    memset(model, 0, sizeof(Model));
    model->meshes = (Mesh**)malloc(sizeof(Mesh*) * meshes_count);
    model->meshes_count = meshes_count;

    uint32_t meshe_start, meshe_end, meshe_size, i, j, children_count, child_index;
    uint8_t* children_indexes;
    glm::mat4* parent_bone_mat;
    Mesh* mesh;

    for(i = 0; i < meshes_count - 1; i++)
    {
        meshe_start = ((uint32_t*)data)[i] << 2;
        meshe_end = (((uint32_t*)data)[i+1] << 2) - 4;
        meshe_size = meshe_end - meshe_start;

        model->meshes[i] = read_mesh_data(&data[meshe_start], meshe_size, model->used_cluts);
        model->meshes[i]->bone_map = &bones_maps[i];
        model->meshes[i]->bone_mat = get_bone_matrix(meshes_rot_angles[i], &meshes_init_pos[i]);
        if(i == 3)
             model->meshes[i]->bone_mat *= get_bone_matrix(shoulders_rot_angles_2[0], &shoulders_init_pos_2[0]);
        else if(i == 8)
             model->meshes[i]->bone_mat *= get_bone_matrix(shoulders_rot_angles_2[1], &shoulders_init_pos_2[1]);
    }

    meshe_start = ((uint32_t*)data)[i] << 2;
    meshe_end = data_size - 4;
    meshe_size = meshe_end - meshe_start;
    model->meshes[i] = read_mesh_data(&data[meshe_start], meshe_size, model->used_cluts);
    model->meshes[i]->bone_map = &bones_maps[i];
    model->meshes[i]->bone_mat = get_bone_matrix(meshes_rot_angles[i], &meshes_init_pos[i]);

    for(i = 0; i < meshes_count; i++)
    {
        children_count = model->meshes[i]->bone_map->children_count;
        children_indexes = model->meshes[i]->bone_map->children_indexes;
        for(j = 0; j < children_count; j++)
        {
            child_index = children_indexes[j] - 1;
            // to overcome padding
            if(child_index > 6) child_index += 1;
            if(child_index > 11) child_index += 1;
            mesh = model->meshes[child_index];
            parent_bone_mat = &model->meshes[i]->bone_mat;
            mesh->bone_mat = *parent_bone_mat * mesh->bone_mat;
        }
    }

    free(data);
    return model;
}

void read_model_meshes(char* data, uint32_t data_size, Model* model)
{
    /*FILE* file = fopen(file_path, "rb");

    if(!file)
        printf("couldn't open file: %s \n", file_path);

    unsigned int data_size = get_file_size(file);
    char* data = (char*)malloc(data_size);
    fread(data, data_size, 1, file);
    fclose(file);*/

    uint32_t meshes_count = ((uint32_t*)data)[0];

    //Model* model = (Model*)malloc(sizeof(Model));
    //memset(model, 0, sizeof(Model));
    model->meshes = (Mesh**)malloc(sizeof(Mesh*) * meshes_count);
    model->meshes_count = meshes_count;

    uint32_t meshe_start, meshe_end, meshe_size, i;

    for(i = 0; i < meshes_count - 1; i++)
    {
        //printf("reading mesh: %d \n", i);
        meshe_start = ((uint32_t*)data)[i] << 2;
        meshe_end = (((uint32_t*)data)[i+1] << 2) - 4;
        meshe_size = meshe_end - meshe_start;

        model->meshes[i] = read_mesh_data(&data[meshe_start], meshe_size, model->used_cluts);
        model->meshes[i]->bone_map = &bones_maps[i];
    }

    meshe_start = ((uint32_t*)data)[i] << 2;
    meshe_end = data_size - 4;
    meshe_size = meshe_end - meshe_start;
    model->meshes[i] = read_mesh_data(&data[meshe_start], meshe_size, model->used_cluts);
    model->meshes[i]->bone_map = &bones_maps[i];

    //free(data);
    //return model;
}

void load_animation_frame(Model* model, uint32_t anim_index, uint32_t frame_index)
{
    /*if(anim_index >= model->animations_count)
        anim_index =  model->animations_count - 1;
    if(frame_index >= model->animations[anim_index]->frames_count)
        frame_index = model->animations[anim_index]->frames_count - 1;*/

    uint32_t i, j, children_count, child_index;
    uint8_t* children_indexes;
    glm::mat4* parent_bone_mat;
    Mesh* mesh;
    BoneRotation** bones_rotations = model->animations[anim_index]->frames[frame_index]->bones_rotations;
    Vec3* axis_shift = &model->animations[anim_index]->frames[frame_index]->axis_shift;
    float** rot  = (float**)&bones_rotations[1];

    glm::mat4 initial_rotation  = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
    initial_rotation = glm::rotate(initial_rotation, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    initial_rotation = glm::rotate(initial_rotation, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 root_axis_shift  = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
    root_axis_shift = glm::translate(root_axis_shift, glm::vec3(axis_shift->x, axis_shift->y, axis_shift->z));

    for(i = j = 0; i < 20; i++)
    {
        model->meshes[i]->bone_mat = get_bone_matrix(rot[j], &model->meshes_init_pos[i]);

        // shoulders needs 2 rotations
        if(i == 3)
             {model->meshes[i]->bone_mat *= get_bone_matrix(rot[j+1], &model->shoulders_init_pos_2[0]);j++;}
        else if(i == 8)
             {model->meshes[i]->bone_mat *= get_bone_matrix(rot[j+1], &model->shoulders_init_pos_2[1]);j++;}

        // to avoid extra hands
        if(i != 6 && i != 7 && i != 11 && i != 12)
                j++;
    }

    // translating waist and lower torso by root axis shift deltas
    model->meshes[0]->bone_mat = root_axis_shift * model->meshes[0]->bone_mat;
    model->meshes[13]->bone_mat = root_axis_shift * model->meshes[13]->bone_mat;

    for(i = 0; i < 20; i++)
    {
        children_count = model->meshes[i]->bone_map->children_count;
        children_indexes = model->meshes[i]->bone_map->children_indexes;
        for(j = 0; j < children_count; j++)
        {
            child_index = children_indexes[j] - 1;
            // to overcome padding
            if(child_index > 6) child_index += 1;
            if(child_index > 11) child_index += 1;
            mesh = model->meshes[child_index];
            parent_bone_mat = &model->meshes[i]->bone_mat;
            mesh->bone_mat = *parent_bone_mat * mesh->bone_mat;
        }
        model->meshes[i]->bone_mat *= initial_rotation;
    }

}

void free_model_meshes(Model* model)
{
    for(int i = 0; i < model->meshes_count; i++)
    {
        free_mesh_2(model->meshes[i]);
    }
    free(model->meshes); model->meshes = NULL;
    free(model); model = NULL;
}

BoneRotation* get_bone_rotation(uint32_t rotation)
{
    BoneRotation* bone_rotation = (BoneRotation*)malloc(sizeof(BoneRotation));
    uint8_t angles = rotation & 0XFF;
    uint8_t precision_1 = ((rotation & 0XFF00) >> 8);
    uint8_t precision_2 = ((rotation & 0XFF0000) >> 16);
    uint8_t precision_3 = ((rotation & 0XFF000000) >> 24);
    static float scale = (360.0 / 4096.0);

    bone_rotation->z_angle = (((angles >> 5) << 8) | precision_1) * 2;
    bone_rotation->y_angle = ((((angles & 0x1C) >> 2) << 8) | precision_2) * 2;
    bone_rotation->x_angle = (((angles & 0x03) << 8) | precision_3) * 4;

    bone_rotation->z_angle *= scale;
    bone_rotation->y_angle *= scale;
    bone_rotation->x_angle *= scale;

    return bone_rotation;
}

void free_bone_rotation(BoneRotation* bone_rotation)
{
    free(bone_rotation); bone_rotation = NULL;
}

AnimFrame* read_animation_frame(char* data, uint32_t frame_start, uint32_t frame_end, uint8_t is_first_frame)
{
    AnimFrame* anim_frame = (AnimFrame*)malloc(sizeof(AnimFrame));
    anim_frame->bones_rotations = (BoneRotation**)malloc(sizeof(BoneRotation*)*23);
    char* data_ptr = &data[frame_start]; //uint8_t* data_ptr_2 = (uint8_t*) &data[frame_start];
    static Vec3 delta_axis_shift; //, initial_axis_shift;
    if(!is_first_frame)
    {
        delta_axis_shift.x += data_ptr[2];
        delta_axis_shift.y += data_ptr[1];
        delta_axis_shift.z += data_ptr[0];

        anim_frame->axis_shift.x = -delta_axis_shift.x;
        anim_frame->axis_shift.y = -delta_axis_shift.y;
        anim_frame->axis_shift.z = -delta_axis_shift.z;
        data_ptr += 3;
    }
    else
    {
        //anim_frame->axis_shift = *((Vec3*)data_ptr);
        /*initial_axis_shift.x = data_ptr_2[1] + (data_ptr_2[0] << 8);
        initial_axis_shift.y = data_ptr_2[3] + (data_ptr_2[2] << 8);
        initial_axis_shift.z = data_ptr_2[5] + (data_ptr_2[4] << 8);
        anim_frame->axis_shift = initial_axis_shift;*/
        delta_axis_shift.x = 0; //-initial_axis_shift.x;
        delta_axis_shift.y = 0; //-initial_axis_shift.y;
        delta_axis_shift.z = 0; //-initial_axis_shift.z;
        data_ptr += 6;
    }

    BoneRotation*  rot;
    for(int i = 0; i < 19; i++)
    {
        anim_frame->bones_rotations[i] = get_bone_rotation(((uint32_t*)data_ptr)[i]);
        rot = anim_frame->bones_rotations[i];
        //printf("bone rotation %d: x=%f y=%f  z=%f \n", i, rot->x_angle, rot->y_angle, rot->z_angle);
    }

    return anim_frame;
}

void free_animation_frame(AnimFrame* anim_frame)
{
    for(int i = 0; i < 19; i++)
        free_bone_rotation(anim_frame->bones_rotations[i]);
    free(anim_frame->bones_rotations);
    anim_frame->bones_rotations = NULL;
    free(anim_frame);
    anim_frame = NULL;
}

Animation* read_animation(char* data, uint32_t* metadata)
{
    Animation* animation = (Animation*)malloc(sizeof(Animation));
    //animation->frames_count = *(uint32_t*)(&data[metadata_start]);
    animation->frames_count = metadata[0]; // + 1;
    //if(animation->frames_count == 0) animation->frames_count++;
    // +1 frame: some animations have one extra frame that they doesn't count it
    animation->frames = (AnimFrame**)malloc(sizeof(AnimFrame*)*animation->frames_count+1);
    printf("frames_count: %d \n", animation->frames_count);

    uint32_t frame_start, frame_end;
    //uint32_t* metadata = (uint32_t*)(&data[metadata_start]);
    uint32_t frame_duration_flag = (metadata[1] & 0xFF) == 2;
    if(!frame_duration_flag && (metadata[1] & 0xFF) != 0)
        printf("Unsupported frame flag: 0x%x \n", metadata[1] & 0xff);
    metadata += 2;

    uint32_t i  = 0;
    animation->frames_count = 0;

    //for(int i = 0; i < animation->frames_count; i++)
    //while((metadata[3] >> 16) != 0x800)
    do
    {
        //printf("metadata[3]: 0x%x 0x%x \n", metadata[3] >> 16,  metadata[3]);
        animation->frames_count++;
        frame_start = metadata[1];
        //printf("frame_addr: 0x%x \n", metadata[1]);
        frame_end = metadata[3];
        animation->frames[i] = read_animation_frame(data, frame_start, frame_end, i == 0);
        animation->frames[i]->duration = 1; //metadata[0];
        if(frame_duration_flag)
            animation->frames[i]->duration += metadata[0];
        //printf("duration %d: %d \n", i, metadata[0]);
        metadata += 2; i++;
    }while((frame_end >> 16) != 0x800);

    return animation;
}

void free_animation(Animation* animation)
{
    for(unsigned int i = 0; i < animation->frames_count; i++)
        free_animation_frame(animation->frames[i]);
    free(animation->frames);
    animation->frames = NULL;
    free(animation);
    animation = NULL;
}

void read_model_animations(char* data, Model* model)
{
    /*FILE* file = fopen(file_path, "rb");

    if(!file)
        printf("couldn't open file: %s \n", file_path);

    unsigned int data_size = get_file_size(file);
    char* data = (char*)malloc(data_size);
    fread(data, data_size, 1, file);
    fclose(file);*/

    uint32_t anim_count = ((uint32_t*)data)[0];
    anim_count /= 4; printf("animations_count: %d \n", anim_count-1);

    model->animations = (Animation**)malloc(sizeof(Animation*)*(anim_count-1));
    // -1 to not count delemiter 0x8000000 at the end of header
    model->animations_count = anim_count - 1; // anim_count - 1;

    uint32_t metadata_start, i, *metadata;

    for(i = 0; i < anim_count - 1; i++) // anim_count - 1;
    {
        printf("animations: %d  ", i);
        metadata_start = ((uint32_t*)data)[i];
        metadata = (uint32_t*)(&data[metadata_start]);
        model->animations[i] = read_animation(data, metadata);
        model->animations[i]->index = i;
    }

    //free(data);
}

void free_model_animations(Model* model)
{
    for(unsigned int i = 0; i < model->animations_count; i++)
        free_animation(model->animations[i]);
    free(model->animations); model->animations = NULL;
}

#define FRAME_INC 3

void update_model_animation(Model* model)
{
    Animation* animation = model->animations[model->current_animation];
    AnimFrame* frame = animation->frames[animation->current_frame];
    frame->time += 1;
    if(frame->time > frame->duration)
    {
        frame->time = 0;
        /*if(animation->current_frame + FRAME_INC >= animation->frames_count)
            animation->current_frame++;
        else*/
            animation->current_frame += FRAME_INC;
        if(animation->current_frame >= animation->frames_count)
            animation->current_frame = 0;
        load_animation_frame(model, model->current_animation, animation->current_frame);
    }
}

void change_model_animation(Model* model, uint32_t animation_index)
{
    if(animation_index <  model->animations_count)
    {
        model->current_animation = animation_index;
        load_animation_frame(model, animation_index, 0);
        //printf("model_current_animation: %d \n", model->current_animation);
    }
    Animation* animation = model->animations[model->current_animation];
    animation->current_frame = 0;
    animation->frames[0]->time = 0;
}

void read_bones_data(char* file_path, uint32_t offset, uint32_t length, Model* model)
{
    FILE* file = fopen(file_path, "rb");

    if(!file)
        printf("couldn't open file: %s \n", file_path);

    unsigned int data_size = get_file_size(file);
    char* data = (char*)malloc(data_size);
    fread(data, data_size, 1, file);
    fclose(file);

    uint16_t* data_ptr = (uint16_t*)(data+offset);
    Vec3* init_pos;

    //printf("\n ... printing bones data ... \n");
    for(int i = 3, j = 0; i < length * 3; i += 3, j++)
    {
        //printf("data %d: x=0x%x  y=0x%x  z=0x%x \n", i/3,
               //data_ptr[i+2], data_ptr[i+1], data_ptr[i]);

        // to avoid extra hands
        //if(i/3 == 6 || i/3 == 11)
        if(i/3 == 8 || i/3 == 13)
            j += 2;

        // shoulders needs 2 translations
        if(i/3 == 5)
        {
            init_pos = &model->shoulders_init_pos_2[0];
            j--;
        }
        else if(i/3 == 9)
        {
            init_pos = &model->shoulders_init_pos_2[1];
            j--;
        }
        else
        {
            init_pos = &model->meshes_init_pos[j];
        }
        init_pos->x = data_ptr[i+2];
        init_pos->y = data_ptr[i+1];
        init_pos->z = data_ptr[i];
    }

    /*printf("\n\n");
    for(int i = 0; i < 25;  i++)
    {
        printf("data %d: x=0x%x  y=0x%x  z=0x%x \n", i,
               meshes_init_pos[i].x, meshes_init_pos[i].y, meshes_init_pos[i].z);
    }

    printf("shoulders_init_pos_2 1: x=0x%x  y=0x%x  z=0x%x \n",
            shoulders_init_pos_2[0].x, shoulders_init_pos_2[0].y, shoulders_init_pos_2[0].z);
    printf("shoulders_init_pos_2 2: x=0x%x  y=0x%x  z=0x%x \n",
            shoulders_init_pos_2[1].x, shoulders_init_pos_2[1].y, shoulders_init_pos_2[1].z);*/

    free(data);
}

void convert_geometry(Model* model)
{
    Mesh* mesh;
    Quad *quad;
    Triangle *tri;
    TriTexcoord* tri_texcoord;
    QuadTexcoord* quad_texcoord;
    FloatTriangle* float_tri;
    FloatQuad* float_quad;
    float tex_size_x = 256;;
    float tex_size_y = model->texture_height;

    for(int i = 0; i < model->meshes_count; i++)
    {
        mesh = model->meshes[i];
        for(int j = 0; j < mesh->triangles_count; j++)
        {
            tri = &mesh->triangles[j];
            tri_texcoord = &mesh->tri_texcoords[j];
            float_tri= &mesh->float_triangles[j];
            tri_texcoord->t1.x = tri->t1.x / tex_size_x; tri_texcoord->t1.y /= tex_size_y;
            tri_texcoord->t2.x = tri->t2.x / tex_size_x; tri_texcoord->t2.y /= tex_size_y;
            tri_texcoord->t3.x = tri->t3.x / tex_size_x; tri_texcoord->t3.y /= tex_size_y;

            float_tri->v1.x = tri->v1.x; float_tri->v1.y = tri->v1.y; float_tri->v1.z = tri->v1.z;
            float_tri->v2.x = tri->v2.x; float_tri->v2.y = tri->v2.y; float_tri->v2.z = tri->v2.z;
            float_tri->v3.x = tri->v3.x; float_tri->v3.y = tri->v3.y; float_tri->v3.z = tri->v3.z;
        }
        for(int j = 0; j < mesh->quads_count; j++)
        {
            quad = &mesh->quads[j];
            quad_texcoord = &mesh->quad_texcoords[j];
            float_quad = &mesh->float_quads[j];
            quad_texcoord->t1.x = quad->t1.x / tex_size_x; quad_texcoord->t1.y /= tex_size_y;
            quad_texcoord->t2.x = quad->t2.x / tex_size_x; quad_texcoord->t2.y /= tex_size_y;
            quad_texcoord->t3.x = quad->t3.x / tex_size_x; quad_texcoord->t3.y /= tex_size_y;
            quad_texcoord->t4.x = quad->t4.x / tex_size_x; quad_texcoord->t4.y /= tex_size_y;

            float_quad->v1.x = quad->v1.x; float_quad->v1.y = quad->v1.y; float_quad->v1.z = quad->v1.z;
            float_quad->v2.x = quad->v2.x; float_quad->v2.y = quad->v2.y; float_quad->v2.z = quad->v2.z;
            float_quad->v3.x = quad->v3.x; float_quad->v3.y = quad->v3.y; float_quad->v3.z = quad->v3.z;
            float_quad->v4.x = quad->v4.x; float_quad->v4.y = quad->v4.y; float_quad->v4.z = quad->v4.z;
        }
    }
}

void get_tri_normal(FloatTriangle* triangle, FloatTriangle* out_normal)
{
    float* v;

    v = &triangle->v1.x;
    glm::vec3 vertex_1(v[0], v[1], v[2]);
    v = &triangle->v2.x;
    glm::vec3 vertex_2(v[0], v[1], v[2]);
    v = &triangle->v3.x;
    glm::vec3 vertex_3(v[0], v[1], v[2]);

    glm::vec3 normal[3], vector_1, vector_2;


    vector_1 = vertex_3 - vertex_1;
    vector_2 = vertex_2 - vertex_1;
    normal[0] = glm::normalize(glm::cross(vector_1, vector_2));

    vector_1 = vertex_1 - vertex_2;
    vector_2 = vertex_3 - vertex_2;
    normal[1] = glm::normalize(glm::cross(vector_1, vector_2));

    vector_1 = vertex_2 - vertex_3;
    vector_2 = vertex_1 - vertex_3;
    normal[2] = glm::normalize(glm::cross(vector_1, vector_2));

    *out_normal = *((FloatTriangle*)normal);
}

void get_quad_normal(FloatQuad* quad, FloatQuad* out_normal)
{
    float* v;

    v = &quad->v1.x;
    glm::vec3 vertex_1(v[0], v[1], v[2]);
    v = &quad->v2.x;
    glm::vec3 vertex_2(v[0], v[1], v[2]);
    v = &quad->v3.x;
    glm::vec3 vertex_3(v[0], v[1], v[2]);
    v = &quad->v4.x;
    glm::vec3 vertex_4(v[0], v[1], v[2]);

    glm::vec3 normal[4], vector_1, vector_2;


    vector_1 = vertex_4 - vertex_1;
    vector_2 = vertex_2 - vertex_1;
    normal[0] = glm::normalize(glm::cross(vector_1, vector_2));

    vector_1 = vertex_1 - vertex_2;
    vector_2 = vertex_3 - vertex_2;
    normal[1] = glm::normalize(glm::cross(vector_1, vector_2));

    vector_1 = vertex_2 - vertex_3;
    vector_2 = vertex_4 - vertex_3;
    normal[2] = glm::normalize(glm::cross(vector_1, vector_2));

    vector_1 = vertex_3 - vertex_4;
    vector_2 = vertex_1 - vertex_4;
    normal[3] = glm::normalize(glm::cross(vector_1, vector_2));

    *out_normal = *((FloatQuad*)normal);
}

void generate_normals(Model* model)
{
    Mesh* mesh;

    for(int i = 0; i  < 20; i++)
    {
        // we don't want to draw extra hands
        if(i == 6 || i == 7 || i == 11 || i == 12)
            continue;

        mesh = model->meshes[i];

        for(int j = 0; j  < mesh->triangles_count; j++)
            get_tri_normal(&mesh->float_triangles[j], &mesh->triangle_normals[j]);

        for(int j = 0; j  < mesh->quads_count; j++)
            get_quad_normal(&mesh->float_quads[j], &mesh->quad_normals[j]);
    }
}

int get_character_id(char* pac_file_path)
{
    char IDs[0x20][3] = {"00", "01", "02", "03", "04", "05",
                         "06", "07", "08", "09", "0A", "0B",
                         "0C", "0D", "0E", "0F", "10", "11",
                         "12", "13", "14", "15", "16", "17",
                         "18", "19"};

    int length = strlen(pac_file_path);
    char id[3];
    id[0] = pac_file_path[length-6];
    id[1] = pac_file_path[length-5];
    id[2] = 0;//'\0';
    for(int i = 0; i < 0x20; i++)
    {
        if(id[0] == IDs[i][0] && id[1] == IDs[i][1])
            return i;
    }
    return -1;
}

Model* load_model(char* pac_file_path)
{
    Pac* pac = load_pac(pac_file_path);
    int id = get_character_id(pac_file_path);
    if(id < 0)
    {
       printf("This is not sfex model ! \n");
       return 0;
    }
    printf("character_id: 0x%02x \n", id);

    Model* model = (Model*)malloc(sizeof(Model));
    memset(model, 0, sizeof(Model));

    read_model_meshes(pac->meshes_data, pac->meshes_data_size, model);
    read_model_animations(pac->animations_data, model);

    for(int i = 0; i < 16; i++)
    {
        printf("clut %d used: %d \n", i, model->used_cluts[i]);
        if(model->used_cluts[i]) model->max_clut_index = i+1;
    }
    printf("max_clut_index: %d \n", model->max_clut_index);
    model->texture_height = 256 * model->max_clut_index;
    printf("texture_height: %d \n", model->texture_height);
    texture_height = model->texture_height;

    model->texture = load_tim(pac->texture_data, true, model->max_clut_index);
    texture = model->texture;

    convert_geometry(model);
    generate_normals(model);

    read_bones_data("lmd/ex_bones_data.bin", bones_metadata[id][0], 0x13, model);

    free(pac);

    return model;
}

void free_model(Model* model)
{
    glDeleteTextures(1, &model->texture);
    free_model_animations(model);
    free_model_meshes(model);
    free(model); model = NULL;
}

void draw_model(Model* model, glm::mat4& model_view_matrix)
{
    Mesh* mesh;
    glColor3ub(255,255,255);
    // bind textures on corresponding texture units
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, model->texture);
    glMatrixMode(GL_MODELVIEW);
    // render model
    for(int i = 0; i  < 20; i++)
    {
        // we don't want to draw extra hands
        if(i == 6 || i == 7 || i == 11 || i == 12)
            continue;

        mesh = model->meshes[i];

        glLoadMatrixf(&model_view_matrix[0][0]);
        glMultMatrixf(&mesh->bone_mat[0][0]);

        glVertexPointer(3, GL_FLOAT, 3*sizeof(GLfloat), (GLfloat*)mesh->float_triangles);
        glTexCoordPointer(2, GL_FLOAT, 2*sizeof(GLfloat), (GLfloat*)mesh->tri_texcoords);
        glDrawArrays(GL_TRIANGLES, 0, mesh->triangles_count * 3);
        //for(int j = 0; j  < mesh->triangles_count; j++)
            //tri_normal_test(&mesh->float_triangles[j]);

        glVertexPointer(3, GL_FLOAT, 3*sizeof(GLfloat), (GLfloat*)mesh->float_quads);
        glTexCoordPointer(2, GL_FLOAT, 2*sizeof(GLfloat), (GLfloat*)mesh->quad_texcoords);
        glDrawArrays(GL_QUADS, 0, mesh->quads_count * 4);
        //for(int j = 0; j  < mesh->quads_count; j++)
            //quad_normal_test(&mesh->float_quads[j]);
    }
    glDisable(GL_TEXTURE_2D);
}

void print_axis_shifts(Model* model, uint32_t anim_index)
{
    AnimFrame** anim_frames = model->animations[anim_index]->frames;
    uint32_t frames_count = model->animations[anim_index]->frames_count;
    Vec3* v;
    printf("\n ... printing axis shifts ... \n");
    for(int i = 0; i  < frames_count; i++)
    {
        v = &anim_frames[i]->axis_shift;
        printf("frame %d axis_shift: x=0x%x  y=0x%x z=0x%x \n", i, v->x, v->y, v->z);
    }

}
