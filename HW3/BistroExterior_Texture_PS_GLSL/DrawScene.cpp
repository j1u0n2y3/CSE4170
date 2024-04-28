//
//  DrawScene.cpp
//
//  Written for CSE4170
//  Department of Computer Science and Engineering
//  Copyright © 2023 Sogang University. All rights reserved.
//

void timer_scene(int value);

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <cctype>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "LoadScene.h"
#include <FreeImage/FreeImage.h>
#include <glm/gtc/matrix_inverse.hpp>
// Begin of shader setup
#include "Shaders/LoadShaders.h"
#include "ShadingInfo.h"

extern SCENE scene;

// for simple shaders
GLuint h_ShaderProgram_simple; // handle to shader program
GLuint h_ShaderProgram_background, h_ShaderProgram_equiToCube;
GLint loc_ModelViewProjectionMatrix, loc_primitive_color; // indices of uniform variables
GLint loc_ModelViewProjectionMatrix_TXPS, loc_ModelViewMatrix_TXPS, loc_ModelViewMatrixInvTrans_TXPS;

// for PBR
GLuint h_ShaderProgram_TXPBR;
#define NUMBER_OF_LIGHT_SUPPORTED 1
GLint loc_global_ambient_color;
GLint loc_lightCount;
loc_light_Parameters loc_light[NUMBER_OF_LIGHT_SUPPORTED];
loc_Material_Parameters loc_material;
GLint loc_ModelViewProjectionMatrix_TXPBR, loc_ModelViewMatrix_TXPBR, loc_ModelViewMatrixInvTrans_TXPBR;
GLint loc_cameraPos;

#define TEXTURE_INDEX_DIFFUSE (0)
#define TEXTURE_INDEX_NORMAL (1)
#define TEXTURE_INDEX_SPECULAR (2)
#define TEXTURE_INDEX_EMISSIVE (3)
#define TEXTURE_INDEX_SKYMAP (4)

// for skybox shaders
GLuint h_ShaderProgram_skybox;
GLint loc_cubemap_skybox;
GLint loc_ModelViewProjectionMatrix_SKY;

// include glm/*.hpp only if necessary
// #include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, lookAt, perspective, etc.
// ViewProjectionMatrix = ProjectionMatrix * ViewMatrix
glm::mat4 ViewProjectionMatrix, ViewMatrix, ProjectionMatrix;
// ModelViewProjectionMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix
glm::mat4 ModelViewProjectionMatrix; // This one is sent to vertex shader when ready.
glm::mat4 ModelViewMatrix;
glm::mat3 ModelViewMatrixInvTrans;

#define TO_RADIAN 0.01745329252f
#define TO_DEGREE 57.295779513f

#define BUFFER_OFFSET(offset) ((GLvoid *)(offset))
#define INDEX_VERTEX_POSITION 0
#define INDEX_NORMAL 1
#define INDEX_TEX_COORD 2
#define LOC_VERTEX 0
#define LOC_NORMAL 1
#define LOC_TEXCOORD 2
int read_geometry(GLfloat **object, int bytes_per_primitive, char *filename)
{
    int n_triangles;
    FILE *fp;

    // fprintf(stdout, "Reading geometry from the geometry file %s...\n", filename);
    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "Cannot open the object file %s ...", filename);
        return -1;
    }
    fread(&n_triangles, sizeof(int), 1, fp);

    *object = (float *)malloc(n_triangles * bytes_per_primitive);
    if (*object == NULL)
    {
        fprintf(stderr, "Cannot allocate memory for the geometry file %s ...", filename);
        return -1;
    }

    fread(*object, bytes_per_primitive, n_triangles, fp);
    // fprintf(stdout, "Read %d primitives successfully.\n\n", n_triangles);
    fclose(fp);

    return n_triangles;
}

/*********************************  START: camera *********************************/
typedef enum
{
    CAMERA_1,
    CAMERA_2,
    CAMERA_3,
    CAMERA_4,
    CAMERA_5,
    CAMERA_6,
    CAMERA_7,
    NUM_CAMERAS
} CAMERA_INDEX;

typedef struct _Camera
{
    glm::vec3 pos;
    glm::vec3 uaxis, vaxis, naxis;
    float fovy, aspect_ratio, near_c, far_c;
    int move, rotation_axis;
} Camera;

Camera camera_info[NUM_CAMERAS];
Camera current_camera;

using glm::mat4;
void set_ViewMatrix_from_camera_frame(void)
{
    ViewMatrix = glm::mat4(current_camera.uaxis[0], current_camera.vaxis[0], current_camera.naxis[0], 0.0f,
                           current_camera.uaxis[1], current_camera.vaxis[1], current_camera.naxis[1], 0.0f,
                           current_camera.uaxis[2], current_camera.vaxis[2], current_camera.naxis[2], 0.0f,
                           0.0f, 0.0f, 0.0f, 1.0f);

    ViewMatrix = glm::translate(ViewMatrix, glm::vec3(-current_camera.pos[0], -current_camera.pos[1], -current_camera.pos[2]));
}

void set_current_camera(int camera_num)
{
    Camera *pCamera = &camera_info[camera_num];

    memcpy(&current_camera, pCamera, sizeof(Camera));
    set_ViewMatrix_from_camera_frame();
    ProjectionMatrix = glm::perspective(current_camera.fovy, current_camera.aspect_ratio, current_camera.near_c, current_camera.far_c);
    if (camera_num == CAMERA_3)
        ProjectionMatrix[0][0] *= -1.0f;
    ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
}

void initialize_camera(void)
{
    Camera *pCamera = &camera_info[CAMERA_1]; // free_cam
    pCamera->pos[0] = -141.351807f;
    pCamera->pos[1] = 1334.346313f;
    pCamera->pos[2] = 243.968231f;
    pCamera->uaxis[0] = 0.998411f;
    pCamera->uaxis[1] = 0.649119f;
    pCamera->uaxis[2] = -0.027553f;
    pCamera->vaxis[0] = -0.028485f;
    pCamera->vaxis[1] = -0.018375f;
    pCamera->vaxis[2] = 0.999417f;
    pCamera->naxis[0] = 0.448585f;
    pCamera->naxis[1] = 1.998617f;
    pCamera->naxis[2] = 0.219746f;
    pCamera->move = 1; // 0? 1?
    pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f;
    pCamera->far_c = 50000.0f;

    /*//CAMERA_1 : original view
    pCamera = &camera_info[CAMERA_1];
    for (int k = 0; k < 3; k++)
    {
        pCamera->pos[k] = scene.camera.e[k];
        pCamera->uaxis[k] = scene.camera.u[k];
        pCamera->vaxis[k] = scene.camera.v[k];
        pCamera->naxis[k] = scene.camera.n[k];
    }

    pCamera->move = 0;
    pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f; pCamera->far_c = 50000.0f;
    */

    // CAMERA_2 : main(tiger,spider)
    pCamera = &camera_info[CAMERA_2];
    pCamera->pos[0] = -376.347595f;
    pCamera->pos[1] = 1246.573975f;
    pCamera->pos[2] = 1207.513916f;
    pCamera->uaxis[0] = -1.003393f;
    pCamera->uaxis[1] = 0.636356;
    pCamera->uaxis[2] = -0.084842f;
    pCamera->vaxis[0] = 0.136137f;
    pCamera->vaxis[1] = 0.236223f;
    pCamera->vaxis[2] = 0.962106f;
    pCamera->naxis[0] = -1.968285f;
    pCamera->naxis[1] = -0.300022f;
    pCamera->naxis[2] = 0.528989f;
    pCamera->move = 0;
    pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f;
    pCamera->far_c = 50000.0f;

    // CAMERA_3 : tiger pov
    pCamera = &camera_info[CAMERA_3];
    pCamera->pos[0] = 0.0f;
    pCamera->pos[1] = -88.0f;
    pCamera->pos[2] = 62.0f;
    pCamera->uaxis[0] = -1.183248f;
    pCamera->uaxis[1] = 0.094224f;
    pCamera->uaxis[2] = -0.099936f;
    pCamera->vaxis[0] = -0.028485f;
    pCamera->vaxis[1] = -0.018375f;
    pCamera->vaxis[2] = 0.999417f;
    pCamera->naxis[0] = 1.583016f;
    pCamera->naxis[1] = 1.314479f;
    pCamera->naxis[2] = -0.100928f;
    pCamera->move = 1;
    pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f;
    pCamera->far_c = 50000.0f;

    // CAMERA_4 : drone view
    pCamera = &camera_info[CAMERA_4];
    pCamera->pos[0] = 5418.375977f;
    pCamera->pos[1] = -3190.530762f;
    pCamera->pos[2] = 2054.045166f;
    pCamera->uaxis[0] = -0.145179f;
    pCamera->uaxis[1] = -1.180712f;
    pCamera->uaxis[2] = 0.061444f;
    pCamera->vaxis[0] = -0.610642f;
    pCamera->vaxis[1] = 0.172813f;
    pCamera->vaxis[2] = 0.772801f;
    pCamera->naxis[0] = 0.775852f;
    pCamera->naxis[1] = -1.507460f;
    pCamera->naxis[2] = 1.170281f;
    pCamera->move = 0;
    pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f;
    pCamera->far_c = 50000.0f;

    // CAMERA_5 : cam2
    pCamera = &camera_info[CAMERA_5];
    pCamera->pos[0] = 2825.625488f;
    pCamera->pos[1] = -2794.181885f;
    pCamera->pos[2] = 742.442322f;
    pCamera->uaxis[0] = -0.747306f;
    pCamera->uaxis[1] = 0.925421f;
    pCamera->uaxis[2] = -0.063795f;
    pCamera->vaxis[0] = 0.210693f;
    pCamera->vaxis[1] = 0.163206f;
    pCamera->vaxis[2] = 0.963822f;
    pCamera->naxis[0] = -1.957322f;
    pCamera->naxis[1] = 0.336447f;
    pCamera->naxis[2] = 0.547401f;
    pCamera->move = 0;
    pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f;
    pCamera->far_c = 50000.0f;

    // CAMERA_6 : cam3
    pCamera = &camera_info[CAMERA_6];
    pCamera->pos[0] = -1218.825195f;
    pCamera->pos[1] = 405.044342f;
    pCamera->pos[2] = 703.734009f;
    pCamera->uaxis[0] = 0.540790f;
    pCamera->uaxis[1] = 1.060848f;
    pCamera->uaxis[2] = -0.033026f;
    pCamera->vaxis[0] = -0.028485f;
    pCamera->vaxis[1] = -0.018375f;
    pCamera->vaxis[2] = 0.999417f;
    pCamera->naxis[0] = -0.612916f;
    pCamera->naxis[1] = 1.957730f;
    pCamera->naxis[2] = 0.188739f;
    pCamera->move = 0;
    pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f;
    pCamera->far_c = 50000.0f;

    // CAMERA_7 : cam4
    pCamera = &camera_info[CAMERA_7];
    pCamera->pos[0] = -566.836426f;
    pCamera->pos[1] = -778.863342f;
    pCamera->pos[2] = 84.022675f;
    pCamera->uaxis[0] = 0.114939f;
    pCamera->uaxis[1] = 1.184860f;
    pCamera->uaxis[2] = -0.042883f;
    pCamera->vaxis[0] = -0.192135f;
    pCamera->vaxis[1] = -0.003154f;
    pCamera->vaxis[2] = 0.981355f;
    pCamera->naxis[0] = -1.305602f;
    pCamera->naxis[1] = 1.591677f;
    pCamera->naxis[2] = -0.077156f;
    pCamera->move = 0;
    pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f;
    pCamera->far_c = 50000.0f;

    set_current_camera(CAMERA_2);
}
/*********************************  END: camera *********************************/

/*************************************static object*************************************************/
// bus object
GLuint bus_VBO, bus_VAO;
int bus_n_triangles;
GLfloat *bus_vertices;
void prepare_bus(void)
{
    int i, n_bytes_per_vertex, n_bytes_per_triangle, bus_n_total_triangles = 0;
    char filename[512];

    n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
    n_bytes_per_triangle = 3 * n_bytes_per_vertex;

    sprintf(filename, "Data/static_objects/bus_vnt.geom");
    bus_n_triangles = read_geometry(&bus_vertices, n_bytes_per_triangle, filename);
    // assume all geometry files are effective
    bus_n_total_triangles += bus_n_triangles;

    // initialize vertex buffer object
    glGenBuffers(1, &bus_VBO);

    glBindBuffer(GL_ARRAY_BUFFER, bus_VBO);
    glBufferData(GL_ARRAY_BUFFER, bus_n_total_triangles * 3 * n_bytes_per_vertex, bus_vertices, GL_STATIC_DRAW);

    // as the geometry data exists now in graphics memory, ...
    free(bus_vertices);

    // initialize vertex array object
    glGenVertexArrays(1, &bus_VAO);
    glBindVertexArray(bus_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, bus_VBO);
    glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // material_bus.ambient_color[0] = 0.24725f;
    // material_bus.ambient_color[1] = 0.1995f;
    // material_bus.ambient_color[2] = 0.0745f;
    // material_bus.ambient_color[3] = 1.0f;
    //
    // material_bus.diffuse_color[0] = 0.75164f;
    // material_bus.diffuse_color[1] = 0.60648f;
    // material_bus.diffuse_color[2] = 0.22648f;
    // material_bus.diffuse_color[3] = 1.0f;
    //
    // material_bus.specular_color[0] = 0.728281f;
    // material_bus.specular_color[1] = 0.655802f;
    // material_bus.specular_color[2] = 0.466065f;
    // material_bus.specular_color[3] = 1.0f;
    //
    // material_bus.specular_exponent = 51.2f;
    //
    // material_bus.emissive_color[0] = 0.1f;
    // material_bus.emissive_color[1] = 0.1f;
    // material_bus.emissive_color[2] = 0.0f;
    // material_bus.emissive_color[3] = 1.0f;

    /*glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

    glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
    glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

    My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);*/
}
void draw_bus(void)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glFrontFace(GL_CW);

    glBindVertexArray(bus_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3 * bus_n_triangles);
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// godzilla object
GLuint godzilla_VBO, godzilla_VAO;
int godzilla_n_triangles;
GLfloat *godzilla_vertices;
void prepare_godzilla(void)
{
    int i, n_bytes_per_vertex, n_bytes_per_triangle, godzilla_n_total_triangles = 0;
    char filename[512];

    n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
    n_bytes_per_triangle = 3 * n_bytes_per_vertex;

    sprintf(filename, "Data/static_objects/godzilla_vnt.geom");
    godzilla_n_triangles = read_geometry(&godzilla_vertices, n_bytes_per_triangle, filename);
    // assume all geometry files are effective
    godzilla_n_total_triangles += godzilla_n_triangles;

    // initialize vertex buffer object
    glGenBuffers(1, &godzilla_VBO);

    glBindBuffer(GL_ARRAY_BUFFER, godzilla_VBO);
    glBufferData(GL_ARRAY_BUFFER, godzilla_n_total_triangles * 3 * n_bytes_per_vertex, godzilla_vertices, GL_STATIC_DRAW);

    // as the geometry data exists now in graphics memory, ...
    free(godzilla_vertices);

    // initialize vertex array object
    glGenVertexArrays(1, &godzilla_VAO);
    glBindVertexArray(godzilla_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, godzilla_VBO);
    glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // material_godzilla.ambient_color[0] = 0.24725f;
    // material_godzilla.ambient_color[1] = 0.1995f;
    // material_godzilla.ambient_color[2] = 0.0745f;
    // material_godzilla.ambient_color[3] = 1.0f;
    //
    // material_godzilla.diffuse_color[0] = 0.75164f;
    // material_godzilla.diffuse_color[1] = 0.60648f;
    // material_godzilla.diffuse_color[2] = 0.22648f;
    // material_godzilla.diffuse_color[3] = 1.0f;
    //
    // material_godzilla.specular_color[0] = 0.728281f;
    // material_godzilla.specular_color[1] = 0.655802f;
    // material_godzilla.specular_color[2] = 0.466065f;
    // material_godzilla.specular_color[3] = 1.0f;
    //
    // material_godzilla.specular_exponent = 51.2f;
    //
    // material_godzilla.emissive_color[0] = 0.1f;
    // material_godzilla.emissive_color[1] = 0.1f;
    // material_godzilla.emissive_color[2] = 0.0f;
    // material_godzilla.emissive_color[3] = 1.0f;

    /*glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

    glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
    glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

    My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);*/
}

void draw_godzilla(void)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glFrontFace(GL_CW);

    glBindVertexArray(godzilla_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3 * godzilla_n_triangles);
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// cow object
GLuint cow_VBO, cow_VAO;
int cow_n_triangles;
GLfloat *cow_vertices;

void prepare_cow(void)
{
    int i, n_bytes_per_vertex, n_bytes_per_triangle, cow_n_total_triangles = 0;
    char filename[512];

    n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
    n_bytes_per_triangle = 3 * n_bytes_per_vertex;

    sprintf(filename, "Data/static_objects/cow_vn.geom");
    cow_n_triangles = read_geometry(&cow_vertices, n_bytes_per_triangle, filename);
    // assume all geometry files are effective
    cow_n_total_triangles += cow_n_triangles;

    // initialize vertex buffer object
    glGenBuffers(1, &cow_VBO);

    glBindBuffer(GL_ARRAY_BUFFER, cow_VBO);
    glBufferData(GL_ARRAY_BUFFER, cow_n_total_triangles * 3 * n_bytes_per_vertex, cow_vertices, GL_STATIC_DRAW);

    // as the geometry data exists now in graphics memory, ...
    free(cow_vertices);

    // initialize vertex array object
    glGenVertexArrays(1, &cow_VAO);
    glBindVertexArray(cow_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, cow_VBO);
    glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // material_cow.ambient_color[0] = 0.24725f;
    // material_cow.ambient_color[1] = 0.1995f;
    // material_cow.ambient_color[2] = 0.0745f;
    // material_cow.ambient_color[3] = 1.0f;
    //
    // material_cow.diffuse_color[0] = 0.75164f;
    // material_cow.diffuse_color[1] = 0.60648f;
    // material_cow.diffuse_color[2] = 0.22648f;
    // material_cow.diffuse_color[3] = 1.0f;
    //
    // material_cow.specular_color[0] = 0.728281f;
    // material_cow.specular_color[1] = 0.655802f;
    // material_cow.specular_color[2] = 0.466065f;
    // material_cow.specular_color[3] = 1.0f;
    //
    // material_cow.specular_exponent = 51.2f;
    //
    // material_cow.emissive_color[0] = 0.1f;
    // material_cow.emissive_color[1] = 0.1f;
    // material_cow.emissive_color[2] = 0.0f;
    // material_cow.emissive_color[3] = 1.0f;
}
void draw_cow(void)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glFrontFace(GL_CW);

    glBindVertexArray(cow_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3 * cow_n_triangles);
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// ironman object
GLuint ironman_VBO, ironman_VAO;
int ironman_n_triangles;
GLfloat *ironman_vertices;
void prepare_ironman(void)
{
    int i, n_bytes_per_vertex, n_bytes_per_triangle, ironman_n_total_triangles = 0;
    char filename[512];

    n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
    n_bytes_per_triangle = 3 * n_bytes_per_vertex;

    sprintf(filename, "Data/static_objects/ironman_vnt.geom");
    ironman_n_triangles = read_geometry(&ironman_vertices, n_bytes_per_triangle, filename);
    // assume all geometry files are effective
    ironman_n_total_triangles += ironman_n_triangles;

    // initialize vertex buffer object
    glGenBuffers(1, &ironman_VBO);

    glBindBuffer(GL_ARRAY_BUFFER, ironman_VBO);
    glBufferData(GL_ARRAY_BUFFER, ironman_n_total_triangles * 3 * n_bytes_per_vertex, ironman_vertices, GL_STATIC_DRAW);

    // as the geometry data exists now in graphics memory, ...
    free(ironman_vertices);

    // initialize vertex array object
    glGenVertexArrays(1, &ironman_VAO);
    glBindVertexArray(ironman_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, ironman_VBO);
    glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // material_ironman.ambient_color[0] = 0.24725f;
    // material_ironman.ambient_color[1] = 0.1995f;
    // material_ironman.ambient_color[2] = 0.0745f;
    // material_ironman.ambient_color[3] = 1.0f;
    //
    // material_ironman.diffuse_color[0] = 0.75164f;
    // material_ironman.diffuse_color[1] = 0.60648f;
    // material_ironman.diffuse_color[2] = 0.22648f;
    // material_ironman.diffuse_color[3] = 1.0f;
    //
    // material_ironman.specular_color[0] = 0.728281f;
    // material_ironman.specular_color[1] = 0.655802f;
    // material_ironman.specular_color[2] = 0.466065f;
    // material_ironman.specular_color[3] = 1.0f;
    //
    // material_ironman.specular_exponent = 51.2f;
    //
    // material_ironman.emissive_color[0] = 0.1f;
    // material_ironman.emissive_color[1] = 0.1f;
    // material_ironman.emissive_color[2] = 0.0f;
    // material_ironman.emissive_color[3] = 1.0f;

    /*glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

    glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
    glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

    My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);*/
}
void draw_ironman(void)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glFrontFace(GL_CW);

    glBindVertexArray(ironman_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3 * ironman_n_triangles);
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// bike object
GLuint bike_VBO, bike_VAO;
int bike_n_triangles;
GLfloat *bike_vertices;
void prepare_bike(void)
{
    int i, n_bytes_per_vertex, n_bytes_per_triangle, bike_n_total_triangles = 0;
    char filename[512];

    n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
    n_bytes_per_triangle = 3 * n_bytes_per_vertex;

    sprintf(filename, "Data/static_objects/bike_vnt.geom");
    bike_n_triangles = read_geometry(&bike_vertices, n_bytes_per_triangle, filename);
    // assume all geometry files are effective
    bike_n_total_triangles += bike_n_triangles;

    // initialize vertex buffer object
    glGenBuffers(1, &bike_VBO);

    glBindBuffer(GL_ARRAY_BUFFER, bike_VBO);
    glBufferData(GL_ARRAY_BUFFER, bike_n_total_triangles * 3 * n_bytes_per_vertex, bike_vertices, GL_STATIC_DRAW);

    // as the geometry data exists now in graphics memory, ...
    free(bike_vertices);

    // initialize vertex array object
    glGenVertexArrays(1, &bike_VAO);
    glBindVertexArray(bike_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, bike_VBO);
    glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // material_bike.ambient_color[0] = 0.24725f;
    // material_bike.ambient_color[1] = 0.1995f;
    // material_bike.ambient_color[2] = 0.0745f;
    // material_bike.ambient_color[3] = 1.0f;
    //
    // material_bike.diffuse_color[0] = 0.75164f;
    // material_bike.diffuse_color[1] = 0.60648f;
    // material_bike.diffuse_color[2] = 0.22648f;
    // material_bike.diffuse_color[3] = 1.0f;
    //
    // material_bike.specular_color[0] = 0.728281f;
    // material_bike.specular_color[1] = 0.655802f;
    // material_bike.specular_color[2] = 0.466065f;
    // material_bike.specular_color[3] = 1.0f;
    //
    // material_bike.specular_exponent = 51.2f;
    //
    // material_bike.emissive_color[0] = 0.1f;
    // material_bike.emissive_color[1] = 0.1f;
    // material_bike.emissive_color[2] = 0.0f;
    // material_bike.emissive_color[3] = 1.0f;

    /*glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

    glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
    glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

    My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);*/
}
void draw_bike(void)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glFrontFace(GL_CW);

    glBindVertexArray(bike_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3 * bike_n_triangles);
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

/******************************  START: shader setup ****************************/
// Begin of Callback function definitions
void prepare_shader_program(void)
{
    char string[256];

    ShaderInfo shader_info[3] = {
        {GL_VERTEX_SHADER, "Shaders/simple.vert"},
        {GL_FRAGMENT_SHADER, "Shaders/simple.frag"},
        {GL_NONE, NULL}};

    h_ShaderProgram_simple = LoadShaders(shader_info);
    glUseProgram(h_ShaderProgram_simple);

    loc_ModelViewProjectionMatrix = glGetUniformLocation(h_ShaderProgram_simple, "u_ModelViewProjectionMatrix");
    loc_primitive_color = glGetUniformLocation(h_ShaderProgram_simple, "u_primitive_color");

    ShaderInfo shader_info_TXPBR[3] = {
        {GL_VERTEX_SHADER, "Shaders/Background/PBR_Tx.vert"},
        {GL_FRAGMENT_SHADER, "Shaders/Background/PBR_Tx.frag"},
        {GL_NONE, NULL}};

    h_ShaderProgram_TXPBR = LoadShaders(shader_info_TXPBR);
    glUseProgram(h_ShaderProgram_TXPBR);

    loc_ModelViewProjectionMatrix_TXPBR = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_ModelViewProjectionMatrix");
    loc_ModelViewMatrix_TXPBR = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_ModelViewMatrix");
    loc_ModelViewMatrixInvTrans_TXPBR = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_ModelViewMatrixInvTrans");

    loc_lightCount = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_light_count");

    for (int i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++)
    {
        sprintf(string, "u_light[%d].position", i);
        loc_light[i].position = glGetUniformLocation(h_ShaderProgram_TXPBR, string);
        sprintf(string, "u_light[%d].color", i);
        loc_light[i].color = glGetUniformLocation(h_ShaderProgram_TXPBR, string);
    }

    loc_cameraPos = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_camPos");

    // Textures
    loc_material.diffuseTex = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_albedoMap");
    loc_material.normalTex = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_normalMap");
    loc_material.specularTex = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_metallicRoughnessMap");
    loc_material.emissiveTex = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_emissiveMap");

    ShaderInfo shader_info_skybox[3] = {
        {GL_VERTEX_SHADER, "Shaders/Background/skybox.vert"},
        {GL_FRAGMENT_SHADER, "Shaders/Background/skybox.frag"},
        {GL_NONE, NULL}};

    h_ShaderProgram_skybox = LoadShaders(shader_info_skybox);
    loc_cubemap_skybox = glGetUniformLocation(h_ShaderProgram_skybox, "u_skymap");
    loc_ModelViewProjectionMatrix_SKY = glGetUniformLocation(h_ShaderProgram_skybox, "u_ModelViewProjectionMatrix");
    /////////////////////////////////////////
}
/*******************************  END: shder setup ******************************/

/****************************  START: geometry setup ****************************/

int flag_tiger_animation = 1; // animation flag

// ben
//  ben object
#define N_BEN_FRAMES 30
GLuint ben_VBO, ben_VAO;
int ben_n_triangles[N_BEN_FRAMES];
int ben_vertex_offset[N_BEN_FRAMES];
GLfloat *ben_vertices[N_BEN_FRAMES];
int cur_frame_ben = 0;

typedef struct _Ben
{
    float x, y, z;
    float angle;
} Ben;
Ben ben_info;

void prepare_ben(void)
{
    ben_info.x = -386.693970f;
    ben_info.y = -692.807983f;
    ben_info.z = 25.0f;
    ben_info.angle = 70;

    int i, n_bytes_per_vertex, n_bytes_per_triangle, ben_n_total_triangles = 0;
    char filename[512];

    n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
    n_bytes_per_triangle = 3 * n_bytes_per_vertex;

    for (i = 0; i < N_BEN_FRAMES; i++)
    {
        sprintf(filename, "Data/dynamic_objects/ben/ben_vn%d%d.geom", i / 10, i % 10);
        ben_n_triangles[i] = read_geometry(&ben_vertices[i], n_bytes_per_triangle, filename);
        // assume all geometry files are effective
        ben_n_total_triangles += ben_n_triangles[i];

        if (i == 0)
            ben_vertex_offset[i] = 0;
        else
            ben_vertex_offset[i] = ben_vertex_offset[i - 1] + 3 * ben_n_triangles[i - 1];
    }

    // initialize vertex buffer object
    glGenBuffers(1, &ben_VBO);

    glBindBuffer(GL_ARRAY_BUFFER, ben_VBO);
    glBufferData(GL_ARRAY_BUFFER, ben_n_total_triangles * n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

    for (i = 0; i < N_BEN_FRAMES; i++)
        glBufferSubData(GL_ARRAY_BUFFER, ben_vertex_offset[i] * n_bytes_per_vertex,
                        ben_n_triangles[i] * n_bytes_per_triangle, ben_vertices[i]);

    // as the geometry data exists now in graphics memory, ...
    for (i = 0; i < N_BEN_FRAMES; i++)
        free(ben_vertices[i]);

    // initialize vertex array object
    glGenVertexArrays(1, &ben_VAO);
    glBindVertexArray(ben_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, ben_VBO);
    glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // material_ben.ambient_color[0] = 0.24725f;
    // material_ben.ambient_color[1] = 0.1995f;
    // material_ben.ambient_color[2] = 0.0745f;
    // material_ben.ambient_color[3] = 1.0f;
    //
    // material_ben.diffuse_color[0] = 0.75164f;
    // material_ben.diffuse_color[1] = 0.60648f;
    // material_ben.diffuse_color[2] = 0.22648f;
    // material_ben.diffuse_color[3] = 1.0f;
    //
    // material_ben.specular_color[0] = 0.728281f;
    // material_ben.specular_color[1] = 0.655802f;
    // material_ben.specular_color[2] = 0.466065f;
    // material_ben.specular_color[3] = 1.0f;
    //
    // material_ben.specular_exponent = 51.2f;
    //
    // material_ben.emissive_color[0] = 0.1f;
    // material_ben.emissive_color[1] = 0.1f;
    // material_ben.emissive_color[2] = 0.0f;
    // material_ben.emissive_color[3] = 1.0f;

    /*glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

    glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
    glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

    My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);*/
}
void draw_ben(void)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glFrontFace(GL_CW);

    glBindVertexArray(ben_VAO);
    glDrawArrays(GL_TRIANGLES, ben_vertex_offset[cur_frame_ben], 3 * ben_n_triangles[cur_frame_ben]);
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// spider
#define N_SPIDER_FRAMES 16
int cur_frame_spider = 0;
GLuint spider_VBO, spider_VAO;
int spider_n_triangles[N_SPIDER_FRAMES];
int spider_vertex_offset[N_SPIDER_FRAMES];
GLfloat *spider_vertices[N_SPIDER_FRAMES];

typedef struct _Spider
{
    float x, y, z;
} Spider;
Spider spider_info;
int spider_chk = 0; // left or right

void prepare_spider(void)
{

    spider_info.z = 693.813347f;

    int i, n_bytes_per_vertex, n_bytes_per_triangle, spider_n_total_triangles = 0;
    char filename[512];

    n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
    n_bytes_per_triangle = 3 * n_bytes_per_vertex;

    for (i = 0; i < N_SPIDER_FRAMES; i++)
    {
        sprintf(filename, "Data/dynamic_objects/spider/spider_vnt_%d%d.geom", i / 10, i % 10);
        spider_n_triangles[i] = read_geometry(&spider_vertices[i], n_bytes_per_triangle, filename);
        // assume all geometry files are effective
        spider_n_total_triangles += spider_n_triangles[i];

        if (i == 0)
            spider_vertex_offset[i] = 0;
        else
            spider_vertex_offset[i] = spider_vertex_offset[i - 1] + 3 * spider_n_triangles[i - 1];
    }

    // initialize vertex buffer object
    glGenBuffers(1, &spider_VBO);

    glBindBuffer(GL_ARRAY_BUFFER, spider_VBO);
    glBufferData(GL_ARRAY_BUFFER, spider_n_total_triangles * n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

    for (i = 0; i < N_SPIDER_FRAMES; i++)
        glBufferSubData(GL_ARRAY_BUFFER, spider_vertex_offset[i] * n_bytes_per_vertex,
                        spider_n_triangles[i] * n_bytes_per_triangle, spider_vertices[i]);

    // as the geometry data exists now in graphics memory, ...
    for (i = 0; i < N_SPIDER_FRAMES; i++)
        free(spider_vertices[i]);

    // initialize vertex array object
    glGenVertexArrays(1, &spider_VAO);
    glBindVertexArray(spider_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, spider_VBO);
    glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // material_spider.ambient_color[0] = 0.24725f;
    // material_spider.ambient_color[1] = 0.1995f;
    // material_spider.ambient_color[2] = 0.0745f;
    // material_spider.ambient_color[3] = 1.0f;
    //
    // material_spider.diffuse_color[0] = 0.75164f;
    // material_spider.diffuse_color[1] = 0.60648f;
    // material_spider.diffuse_color[2] = 0.22648f;
    // material_spider.diffuse_color[3] = 1.0f;
    //
    // material_spider.specular_color[0] = 0.728281f;
    // material_spider.specular_color[1] = 0.655802f;
    // material_spider.specular_color[2] = 0.466065f;
    // material_spider.specular_color[3] = 1.0f;
    //
    // material_spider.specular_exponent = 51.2f;
    //
    // material_spider.emissive_color[0] = 0.1f;
    // material_spider.emissive_color[1] = 0.1f;
    // material_spider.emissive_color[2] = 0.0f;
    // material_spider.emissive_color[3] = 1.0f;

    /*glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

    glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
    glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

    My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);*/
}

void draw_spider(void)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glFrontFace(GL_CW);

    glBindVertexArray(spider_VAO);
    glDrawArrays(GL_TRIANGLES, spider_vertex_offset[cur_frame_spider], 3 * spider_n_triangles[cur_frame_spider]);
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// tiger

typedef struct _Tiger
{
    float x, y, z;
    float angle;
} Tiger;
Tiger tiger_info;

#define N_TIGER_FRAMES 12
int cur_frame_tiger = 0;
unsigned int timestamp_scene = 0; // the global clock in the scene
float rotation_angle_tiger = 0.0f;
GLuint tiger_VBO, tiger_VAO;
int tiger_n_triangles[N_TIGER_FRAMES];
int tiger_vertex_offset[N_TIGER_FRAMES];
GLfloat *tiger_vertices[N_TIGER_FRAMES];

void prepare_tiger(void)
{ // vertices enumerated clockwise

    tiger_info.x = 554.873474f;
    tiger_info.y = 1999.583862f;
    tiger_info.z = 20.31f;
    tiger_info.angle = 150;

    int i, n_bytes_per_vertex, n_bytes_per_triangle, tiger_n_total_triangles = 0;
    char filename[512];

    n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
    n_bytes_per_triangle = 3 * n_bytes_per_vertex;

    for (i = 0; i < N_TIGER_FRAMES; i++)
    {
        sprintf(filename, "Data/dynamic_objects/tiger/Tiger_%d%d_triangles_vnt.geom", i / 10, i % 10);
        tiger_n_triangles[i] = read_geometry(&tiger_vertices[i], n_bytes_per_triangle, filename);
        // assume all geometry files are effective
        tiger_n_total_triangles += tiger_n_triangles[i];

        if (i == 0)
            tiger_vertex_offset[i] = 0;
        else
            tiger_vertex_offset[i] = tiger_vertex_offset[i - 1] + 3 * tiger_n_triangles[i - 1];
    }

    // initialize vertex buffer object
    glGenBuffers(1, &tiger_VBO);

    glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
    glBufferData(GL_ARRAY_BUFFER, tiger_n_total_triangles * n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

    for (i = 0; i < N_TIGER_FRAMES; i++)
        glBufferSubData(GL_ARRAY_BUFFER, tiger_vertex_offset[i] * n_bytes_per_vertex,
                        tiger_n_triangles[i] * n_bytes_per_triangle, tiger_vertices[i]);

    // as the geometry data exists now in graphics memory, ...
    for (i = 0; i < N_TIGER_FRAMES; i++)
        free(tiger_vertices[i]);

    // initialize vertex array object
    glGenVertexArrays(1, &tiger_VAO);
    glBindVertexArray(tiger_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
    glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    /*material_tiger.ambient_color[0] = 0.24725f;
    material_tiger.ambient_color[1] = 0.1995f;
    material_tiger.ambient_color[2] = 0.0745f;
    material_tiger.ambient_color[3] = 1.0f;

    material_tiger.diffuse_color[0] = 0.75164f;
    material_tiger.diffuse_color[1] = 0.60648f;
    material_tiger.diffuse_color[2] = 0.22648f;
    material_tiger.diffuse_color[3] = 1.0f;

    material_tiger.specular_color[0] = 0.728281f;
    material_tiger.specular_color[1] = 0.655802f;
    material_tiger.specular_color[2] = 0.466065f;
    material_tiger.specular_color[3] = 1.0f;

    material_tiger.specular_exponent = 51.2f;

    material_tiger.emissive_color[0] = 0.1f;
    material_tiger.emissive_color[1] = 0.1f;
    material_tiger.emissive_color[2] = 0.0f;
    material_tiger.emissive_color[3] = 1.0f;

    glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
    /*
    glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
    glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

    My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);*/
}

void draw_tiger(void)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glFrontFace(GL_CW);

    glBindVertexArray(tiger_VAO);
    glDrawArrays(GL_TRIANGLES, tiger_vertex_offset[cur_frame_tiger], 3 * tiger_n_triangles[cur_frame_tiger]);
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

bool b_draw_grid = false;

// axes
GLuint axes_VBO, axes_VAO;
GLfloat axes_vertices[6][3] = {
    {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
GLfloat axes_color[3][3] = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};

void prepare_axes(void)
{
    // Initialize vertex buffer object.
    glGenBuffers(1, &axes_VBO);

    glBindBuffer(GL_ARRAY_BUFFER, axes_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axes_vertices), &axes_vertices[0][0], GL_STATIC_DRAW);

    // Initialize vertex array object.
    glGenVertexArrays(1, &axes_VAO);
    glBindVertexArray(axes_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, axes_VBO);
    glVertexAttribPointer(INDEX_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(INDEX_VERTEX_POSITION);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    fprintf(stdout, " * Loaded axes into graphics memory.\n");
}

void draw_axes(void)
{
    if (!b_draw_grid)
        return;

    glUseProgram(h_ShaderProgram_simple);
    ModelViewMatrix = glm::scale(ViewMatrix, glm::vec3(8000.0f, 8000.0f, 8000.0f));
    ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
    glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
    glLineWidth(2.0f);
    glBindVertexArray(axes_VAO);
    glUniform3fv(loc_primitive_color, 1, axes_color[0]);
    glDrawArrays(GL_LINES, 0, 2);
    glUniform3fv(loc_primitive_color, 1, axes_color[1]);
    glDrawArrays(GL_LINES, 2, 2);
    glUniform3fv(loc_primitive_color, 1, axes_color[2]);
    glDrawArrays(GL_LINES, 4, 2);
    glBindVertexArray(0);
    glLineWidth(1.0f);
    glUseProgram(0);
}

// grid
#define GRID_LENGTH (100)
#define NUM_GRID_VETICES ((2 * GRID_LENGTH + 1) * 4)
GLuint grid_VBO, grid_VAO;
GLfloat grid_vertices[NUM_GRID_VETICES][3];
GLfloat grid_color[3] = {0.5f, 0.5f, 0.5f};

void prepare_grid(void)
{

    // set grid vertices
    int vertex_idx = 0;
    for (int x_idx = -GRID_LENGTH; x_idx <= GRID_LENGTH; x_idx++)
    {
        grid_vertices[vertex_idx][0] = x_idx;
        grid_vertices[vertex_idx][1] = -GRID_LENGTH;
        grid_vertices[vertex_idx][2] = 0.0f;
        vertex_idx++;

        grid_vertices[vertex_idx][0] = x_idx;
        grid_vertices[vertex_idx][1] = GRID_LENGTH;
        grid_vertices[vertex_idx][2] = 0.0f;
        vertex_idx++;
    }

    for (int y_idx = -GRID_LENGTH; y_idx <= GRID_LENGTH; y_idx++)
    {
        grid_vertices[vertex_idx][0] = -GRID_LENGTH;
        grid_vertices[vertex_idx][1] = y_idx;
        grid_vertices[vertex_idx][2] = 0.0f;
        vertex_idx++;

        grid_vertices[vertex_idx][0] = GRID_LENGTH;
        grid_vertices[vertex_idx][1] = y_idx;
        grid_vertices[vertex_idx][2] = 0.0f;
        vertex_idx++;
    }

    // Initialize vertex buffer object.
    glGenBuffers(1, &grid_VBO);

    glBindBuffer(GL_ARRAY_BUFFER, grid_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(grid_vertices), &grid_vertices[0][0], GL_STATIC_DRAW);

    // Initialize vertex array object.
    glGenVertexArrays(1, &grid_VAO);
    glBindVertexArray(grid_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, grid_VAO);
    glVertexAttribPointer(INDEX_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(INDEX_VERTEX_POSITION);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    fprintf(stdout, " * Loaded grid into graphics memory.\n");
}

void draw_grid(void)
{
    if (!b_draw_grid)
        return;

    glUseProgram(h_ShaderProgram_simple);
    ModelViewMatrix = glm::scale(ViewMatrix, glm::vec3(100.0f, 100.0f, 100.0f));
    ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
    glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
    glLineWidth(1.0f);
    glBindVertexArray(grid_VAO);
    glUniform3fv(loc_primitive_color, 1, grid_color);
    glDrawArrays(GL_LINES, 0, NUM_GRID_VETICES);
    glBindVertexArray(0);
    glLineWidth(1.0f);
    glUseProgram(0);
}

// bistro_exterior
GLuint *bistro_exterior_VBO;
GLuint *bistro_exterior_VAO;
int *bistro_exterior_n_triangles;
int *bistro_exterior_vertex_offset;
GLfloat **bistro_exterior_vertices;
GLuint *bistro_exterior_texture_names;

int flag_fog;
bool *flag_texture_mapping;

void initialize_lights(void)
{ // follow OpenGL conventions for initialization
    glUseProgram(h_ShaderProgram_TXPBR);

    glUniform1f(loc_lightCount, scene.n_lights);

    for (int i = 0; i < scene.n_lights; i++)
    {
        glUniform4f(loc_light[i].position,
                    scene.light_list[i].pos[0],
                    scene.light_list[i].pos[1],
                    scene.light_list[i].pos[2],
                    0.0f);

        glUniform3f(loc_light[i].color,
                    scene.light_list[i].color[0],
                    scene.light_list[i].color[1],
                    scene.light_list[i].color[2]);
    }

    glUseProgram(0);
}

bool readTexImage2D_from_file(char *filename)
{
    FREE_IMAGE_FORMAT tx_file_format;
    int tx_bits_per_pixel;
    FIBITMAP *tx_pixmap, *tx_pixmap_32;

    int width, height;
    GLvoid *data;

    tx_file_format = FreeImage_GetFileType(filename, 0);
    // assume everything is fine with reading texture from file: no error checking
    tx_pixmap = FreeImage_Load(tx_file_format, filename);
    if (tx_pixmap == NULL)
        return false;
    tx_bits_per_pixel = FreeImage_GetBPP(tx_pixmap);

    // fprintf(stdout, " * A %d-bit texture was read from %s.\n", tx_bits_per_pixel, filename);
    GLenum format, internalFormat;
    if (tx_bits_per_pixel == 32)
    {
        format = GL_BGRA;
        internalFormat = GL_RGBA;
    }
    else if (tx_bits_per_pixel == 24)
    {
        format = GL_BGR;
        internalFormat = GL_RGB;
    }
    else
    {
        fprintf(stdout, " * Converting texture from %d bits to 32 bits...\n", tx_bits_per_pixel);
        tx_pixmap = FreeImage_ConvertTo32Bits(tx_pixmap);
        format = GL_BGRA;
        internalFormat = GL_RGBA;
    }

    width = FreeImage_GetWidth(tx_pixmap);
    height = FreeImage_GetHeight(tx_pixmap);
    data = FreeImage_GetBits(tx_pixmap);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    // fprintf(stdout, " * Loaded %dx%d RGBA texture into graphics memory.\n\n", width, height);

    FreeImage_Unload(tx_pixmap);

    return true;
}

void prepare_bistro_exterior(void)
{
    int n_bytes_per_vertex, n_bytes_per_triangle;
    char filename[512];

    n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
    n_bytes_per_triangle = 3 * n_bytes_per_vertex;

    // VBO, VAO malloc
    bistro_exterior_VBO = (GLuint *)malloc(sizeof(GLuint) * scene.n_materials);
    bistro_exterior_VAO = (GLuint *)malloc(sizeof(GLuint) * scene.n_materials);

    bistro_exterior_n_triangles = (int *)malloc(sizeof(int) * scene.n_materials);
    bistro_exterior_vertex_offset = (int *)malloc(sizeof(int) * scene.n_materials);

    flag_texture_mapping = (bool *)malloc(sizeof(bool) * scene.n_textures);

    // vertices
    bistro_exterior_vertices = (GLfloat **)malloc(sizeof(GLfloat *) * scene.n_materials);

    for (int materialIdx = 0; materialIdx < scene.n_materials; materialIdx++)
    {
        MATERIAL *pMaterial = &(scene.material_list[materialIdx]);
        GEOMETRY_TRIANGULAR_MESH *tm = &(pMaterial->geometry.tm);

        // vertex
        bistro_exterior_vertices[materialIdx] = (GLfloat *)malloc(sizeof(GLfloat) * 8 * tm->n_triangle * 3);

        int vertexIdx = 0;
        for (int triIdx = 0; triIdx < tm->n_triangle; triIdx++)
        {
            TRIANGLE tri = tm->triangle_list[triIdx];
            for (int triVertex = 0; triVertex < 3; triVertex++)
            {
                bistro_exterior_vertices[materialIdx][vertexIdx++] = tri.position[triVertex].x;
                bistro_exterior_vertices[materialIdx][vertexIdx++] = tri.position[triVertex].y;
                bistro_exterior_vertices[materialIdx][vertexIdx++] = tri.position[triVertex].z;

                bistro_exterior_vertices[materialIdx][vertexIdx++] = tri.normal_vetcor[triVertex].x;
                bistro_exterior_vertices[materialIdx][vertexIdx++] = tri.normal_vetcor[triVertex].y;
                bistro_exterior_vertices[materialIdx][vertexIdx++] = tri.normal_vetcor[triVertex].z;

                bistro_exterior_vertices[materialIdx][vertexIdx++] = tri.texture_list[triVertex][0].u;
                bistro_exterior_vertices[materialIdx][vertexIdx++] = tri.texture_list[triVertex][0].v;
            }
        }

        // # of triangles
        bistro_exterior_n_triangles[materialIdx] = tm->n_triangle;

        if (materialIdx == 0)
            bistro_exterior_vertex_offset[materialIdx] = 0;
        else
            bistro_exterior_vertex_offset[materialIdx] = bistro_exterior_vertex_offset[materialIdx - 1] + 3 * bistro_exterior_n_triangles[materialIdx - 1];

        glGenBuffers(1, &bistro_exterior_VBO[materialIdx]);

        glBindBuffer(GL_ARRAY_BUFFER, bistro_exterior_VBO[materialIdx]);
        glBufferData(GL_ARRAY_BUFFER, bistro_exterior_n_triangles[materialIdx] * 3 * n_bytes_per_vertex,
                     bistro_exterior_vertices[materialIdx], GL_STATIC_DRAW);

        // As the geometry data exists now in graphics memory, ...
        free(bistro_exterior_vertices[materialIdx]);

        // Initialize vertex array object.
        glGenVertexArrays(1, &bistro_exterior_VAO[materialIdx]);
        glBindVertexArray(bistro_exterior_VAO[materialIdx]);

        glBindBuffer(GL_ARRAY_BUFFER, bistro_exterior_VBO[materialIdx]);
        glVertexAttribPointer(INDEX_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(0));
        glEnableVertexAttribArray(INDEX_VERTEX_POSITION);
        glVertexAttribPointer(INDEX_NORMAL, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(3 * sizeof(float)));
        glEnableVertexAttribArray(INDEX_NORMAL);
        glVertexAttribPointer(INDEX_TEX_COORD, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(6 * sizeof(float)));
        glEnableVertexAttribArray(INDEX_TEX_COORD);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        if ((materialIdx > 0) && (materialIdx % 100 == 0))
            fprintf(stdout, " * Loaded %d bistro exterior materials into graphics memory.\n", materialIdx / 100 * 100);
    }
    fprintf(stdout, " * Loaded %d bistro exterior materials into graphics memory.\n", scene.n_materials);

    // textures
    bistro_exterior_texture_names = (GLuint *)malloc(sizeof(GLuint) * scene.n_textures);
    glGenTextures(scene.n_textures, bistro_exterior_texture_names);

    for (int texId = 0; texId < scene.n_textures; texId++)
    {
        glBindTexture(GL_TEXTURE_2D, bistro_exterior_texture_names[texId]);

        bool bReturn = readTexImage2D_from_file(scene.texture_file_name[texId]);

        if (bReturn)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            // glGenerateMipmap(GL_TEXTURE_2D);
            flag_texture_mapping[texId] = true;
        }
        else
        {
            flag_texture_mapping[texId] = false;
        }

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    fprintf(stdout, " * Loaded bistro exterior textures into graphics memory.\n");

    free(bistro_exterior_vertices);
}

void bindTexture(GLuint tex, int glTextureId, int texId)
{
    if (INVALID_TEX_ID != texId)
    {
        glActiveTexture(GL_TEXTURE0 + glTextureId);
        glBindTexture(GL_TEXTURE_2D, bistro_exterior_texture_names[texId]);
        glUniform1i(tex, glTextureId);
    }
}

void draw_bistro_exterior(void)
{
    glUseProgram(h_ShaderProgram_TXPBR);
    ModelViewMatrix = ViewMatrix;
    ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
    ModelViewMatrixInvTrans = glm::transpose(glm::inverse(glm::mat3(ModelViewMatrix)));

    glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPBR, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
    glUniformMatrix4fv(loc_ModelViewMatrix_TXPBR, 1, GL_FALSE, &ModelViewMatrix[0][0]);
    glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPBR, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);

    // glUniform4fv(loc_cameraPos, 1, current_camera.pos);

    for (int materialIdx = 0; materialIdx < scene.n_materials; materialIdx++)
    {
        int diffuseTexId = scene.material_list[materialIdx].diffuseTexId;
        int normalMapTexId = scene.material_list[materialIdx].normalMapTexId;
        int specularTexId = scene.material_list[materialIdx].specularTexId;
        ;
        int emissiveTexId = scene.material_list[materialIdx].emissiveTexId;

        bindTexture(loc_material.diffuseTex, TEXTURE_INDEX_DIFFUSE, diffuseTexId);
        bindTexture(loc_material.normalTex, TEXTURE_INDEX_NORMAL, normalMapTexId);
        bindTexture(loc_material.specularTex, TEXTURE_INDEX_SPECULAR, specularTexId);
        bindTexture(loc_material.emissiveTex, TEXTURE_INDEX_EMISSIVE, emissiveTexId);
        glEnable(GL_TEXTURE_2D);

        glBindVertexArray(bistro_exterior_VAO[materialIdx]);
        glDrawArrays(GL_TRIANGLES, 0, 3 * bistro_exterior_n_triangles[materialIdx]);

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glUseProgram(0);
}

// skybox
GLuint skybox_VBO, skybox_VAO;
GLuint skybox_texture_name;

GLfloat cube_vertices[72][3] = {
    // vertices enumerated clockwise
    // 6*2*3 * 2 (POS & NORM)

    // position
    -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, // right
    1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, // left
    1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,

    -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // top
    1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,

    -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, // bottom
    1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, // back
    -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,

    1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // front
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f,

    // normal
    0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f,
    0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f,

    -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

    1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,

    0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

    0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f,
    0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f};

void readTexImage2DForCubeMap(const char *filename, GLenum texture_target)
{
    FREE_IMAGE_FORMAT tx_file_format;
    int tx_bits_per_pixel;
    FIBITMAP *tx_pixmap;

    int width, height;
    GLvoid *data;

    tx_file_format = FreeImage_GetFileType(filename, 0);
    // assume everything is fine with reading texture from file: no error checking
    tx_pixmap = FreeImage_Load(tx_file_format, filename);
    tx_bits_per_pixel = FreeImage_GetBPP(tx_pixmap);

    // fprintf(stdout, " * A %d-bit texture was read from %s.\n", tx_bits_per_pixel, filename);

    width = FreeImage_GetWidth(tx_pixmap);
    height = FreeImage_GetHeight(tx_pixmap);
    FreeImage_FlipVertical(tx_pixmap);
    data = FreeImage_GetBits(tx_pixmap);

    glTexImage2D(texture_target, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
    // fprintf(stdout, " * Loaded %dx%d RGBA texture into graphics memory.\n\n", width, height);

    FreeImage_Unload(tx_pixmap);
}

void prepare_skybox(void)
{ // Draw skybox.
    glGenVertexArrays(1, &skybox_VAO);
    glGenBuffers(1, &skybox_VBO);

    glBindVertexArray(skybox_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, skybox_VBO);
    glBufferData(GL_ARRAY_BUFFER, 36 * 3 * sizeof(GLfloat), &cube_vertices[0][0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(INDEX_VERTEX_POSITION);
    glVertexAttribPointer(INDEX_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), BUFFER_OFFSET(0));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenTextures(1, &skybox_texture_name);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture_name);

    readTexImage2DForCubeMap("Scene/Cubemap/px.png", GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    readTexImage2DForCubeMap("Scene/Cubemap/nx.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
    readTexImage2DForCubeMap("Scene/Cubemap/py.png", GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
    readTexImage2DForCubeMap("Scene/Cubemap/ny.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
    readTexImage2DForCubeMap("Scene/Cubemap/pz.png", GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
    readTexImage2DForCubeMap("Scene/Cubemap/nz.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
    fprintf(stdout, " * Loaded cube map textures into graphics memory.\n\n");

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void draw_skybox(void)
{
    glUseProgram(h_ShaderProgram_skybox);

    glUniform1i(loc_cubemap_skybox, TEXTURE_INDEX_SKYMAP);

    ModelViewMatrix = ViewMatrix * glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                                             0.0f, 0.0f, 1.0f, 0.0f,
                                             0.0f, 1.0f, 0.0f, 0.0f,
                                             0.0f, 0.0f, 0.0f, 1.0f);
    ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(20000, 20000, 20000));
    // ModelViewMatrix = glm::scale(ViewMatrix, glm::vec3(20000.0f, 20000.0f, 20000.0f));
    ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
    ModelViewMatrixInvTrans = glm::transpose(glm::inverse(glm::mat3(ModelViewMatrix)));

    glUniformMatrix4fv(loc_ModelViewProjectionMatrix_SKY, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);

    glBindVertexArray(skybox_VAO);
    glActiveTexture(GL_TEXTURE0 + TEXTURE_INDEX_SKYMAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture_name);

    glFrontFace(GL_CW);
    glDrawArrays(GL_TRIANGLES, 0, 6 * 2 * 3);
    glBindVertexArray(0);
    glDisable(GL_CULL_FACE);
    glUseProgram(0);
}
/*****************************  END: geometry setup *****************************/

/********************  START: callback function definitions *********************/
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_grid();
    draw_axes();
    draw_bistro_exterior();
    draw_skybox();

    glUseProgram(h_ShaderProgram_simple);
    ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(tiger_info.x, tiger_info.y, tiger_info.z));
    ModelViewMatrix = glm::rotate(ModelViewMatrix, tiger_info.angle * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
    ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
    glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
    draw_tiger();

    glUseProgram(h_ShaderProgram_simple);
    if (spider_chk == 0)
    {
        ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(598.684082f, 2873.774658f, spider_info.z));
        ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(100.0f, 100.0f, 100.0f));
        ModelViewMatrix = glm::rotate(ModelViewMatrix, 50 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
    }
    else
    {
        ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(1665.853760f, 3207.652344f, spider_info.z));
        ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(100.0f, 100.0f, 100.0f));
        ModelViewMatrix = glm::rotate(ModelViewMatrix, 230 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
    }
    ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
    glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
    draw_spider();

    glUseProgram(h_ShaderProgram_simple);
    ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(ben_info.x, ben_info.y, ben_info.z));
    ModelViewMatrix = glm::rotate(ModelViewMatrix, ben_info.angle * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
    ModelViewMatrix = glm::rotate(ModelViewMatrix, 260 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
    ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(250.0f, 250.0f, 250.0f));
    ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
    glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
    draw_ben();

    glUseProgram(h_ShaderProgram_simple);
    ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-134.961639f, 16.743753f, 465.389984f));
    ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(50.0f, 50.0f, 50.0f));
    ModelViewMatrix = glm::rotate(ModelViewMatrix, -20 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
    ModelViewMatrix = glm::rotate(ModelViewMatrix, 90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
    ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
    glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
    draw_ironman();

    glUseProgram(h_ShaderProgram_simple);
    ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-478.696381f, -920.494324f, 35.740677f));
    ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(50.0f, 50.0f, 50.0f));
    ModelViewMatrix = glm::rotate(ModelViewMatrix, 90 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
    ModelViewMatrix = glm::rotate(ModelViewMatrix, 90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
    ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
    glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
    draw_bike();

    glUseProgram(h_ShaderProgram_simple);
    ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(4286.377930f, -1333.481934f, 120.984497f));
    ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(300.0f, 300.0f, 300.0f));
    ModelViewMatrix = glm::rotate(ModelViewMatrix, 170 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
    ModelViewMatrix = glm::rotate(ModelViewMatrix, 90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
    ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
    glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
    draw_cow();

    glUseProgram(h_ShaderProgram_simple);
    ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(1872.686646f, 3786.297363f, 20.984497f));
    ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(2.0f, 2.0f, 2.0f));
    ModelViewMatrix = glm::rotate(ModelViewMatrix, 260 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
    ModelViewMatrix = glm::rotate(ModelViewMatrix, 90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
    ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
    glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
    draw_godzilla();

    glUseProgram(h_ShaderProgram_simple);
    ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(5406.894531f, -3015.435791f, 50.984497f));
    ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(30.0f, 30.0f, 30.0f));
    ModelViewMatrix = glm::rotate(ModelViewMatrix, 155 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
    ModelViewMatrix = glm::rotate(ModelViewMatrix, 90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
    ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
    glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
    draw_bus();

    glutSwapBuffers();
}

int specialKey;
int x_rotate = 0;
int y_rotate = 0;
int x_move = 0, y_move = 0;
int prevx, prevy;
int is_free_cam = 0;
int c_cam = 2;
int c_check = 1;
void mouse(int button, int state, int x, int y)
{
    specialKey = glutGetModifiers();
    if (specialKey == GLUT_ACTIVE_CTRL && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        y_rotate = 1;
        prevy = y;
        prevx = x;
    }
    else if (specialKey == GLUT_ACTIVE_CTRL && button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
    {
        x_rotate = 1;
        prevy = y;
        prevx = x;
    }
    else if (specialKey == GLUT_ACTIVE_SHIFT && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        y_move = 1;
        prevy = y;
        prevx = x;
    }
    else if (specialKey == GLUT_ACTIVE_SHIFT && button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
    {
        x_move = 1;
        prevy = y;
        prevx = x;
    }
    else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
        y_rotate = y_move = 0;
    else if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
        x_rotate = x_move = 0;
}

void motion(int x, int y)
{
    glm::mat3 RotationMatrix;
    specialKey = glutGetModifiers();
    int dx = prevx - x, dy = y - prevy;
    if (is_free_cam == 1)
    {
        if (y_rotate == 1)
        {
            RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0f), dx * 0.5f * TO_RADIAN, camera_info[CAMERA_1].vaxis));
            camera_info[CAMERA_1].uaxis = RotationMatrix * camera_info[CAMERA_1].uaxis;
            camera_info[CAMERA_1].naxis = RotationMatrix * camera_info[CAMERA_1].naxis;
            set_current_camera(CAMERA_1);
            glutPostRedisplay();
        }
        else if (x_rotate == 1)
        {
            RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0f), -dy * 0.5f * TO_RADIAN, camera_info[CAMERA_1].uaxis));
            camera_info[CAMERA_1].vaxis = RotationMatrix * camera_info[CAMERA_1].vaxis;
            camera_info[CAMERA_1].naxis = RotationMatrix * camera_info[CAMERA_1].naxis;
            set_current_camera(CAMERA_1);
            glutPostRedisplay();
        }
        else if (y_move == 1)
        {
            camera_info[CAMERA_1].pos += (dy * 10.0f * camera_info[CAMERA_1].vaxis);
            set_current_camera(CAMERA_1);
            glutPostRedisplay();
        }
        else if (x_move == 1)
        {
            camera_info[CAMERA_1].pos += (dx * 10.0f * camera_info[CAMERA_1].uaxis);
            set_current_camera(CAMERA_1);
            glutPostRedisplay();
        }
    }
    prevx = x;
    prevy = y;
}

void wheel(int wheel, int direction, int x, int y)
{
    specialKey = glutGetModifiers();
    if (is_free_cam == 1 && specialKey == GLUT_ACTIVE_CTRL)
    {
        if (direction > 0)
        {
            camera_info[CAMERA_1].fovy -= 5.0f * TO_RADIAN;
            // camera_info[CAMERA_1].near_c -= 0.1f;
            // camera_info[CAMERA_1].far_c -= 0.1f;
            if (camera_info[CAMERA_1].fovy < 1 * TO_RADIAN)
                camera_info[CAMERA_1].fovy = 1 * TO_RADIAN;
            set_current_camera(CAMERA_1);
            glutPostRedisplay();
        }
        else
        {
            camera_info[CAMERA_1].fovy += 5.0f * TO_RADIAN;
            // camera_info[CAMERA_1].near_c += 0.1f;
            // camera_info[CAMERA_1].far_c += 0.1f;
            if (camera_info[CAMERA_1].fovy > 179 * TO_RADIAN)
                camera_info[CAMERA_1].fovy = 179 * TO_RADIAN;
            set_current_camera(CAMERA_1);
            glutPostRedisplay();
        }
    }
    if (c_cam == 2 && specialKey == GLUT_ACTIVE_CTRL)
    {
        if (direction > 0)
        {
            camera_info[CAMERA_2].fovy -= 5.0f * TO_RADIAN;
            // camera_info[CAMERA_1].near_c -= 0.1f;
            // camera_info[CAMERA_1].far_c -= 0.1f;
            if (camera_info[CAMERA_2].fovy < 1 * TO_RADIAN)
                camera_info[CAMERA_2].fovy = 1 * TO_RADIAN;
            set_current_camera(CAMERA_2);
            glutPostRedisplay();
        }
        else
        {
            camera_info[CAMERA_2].fovy += 5.0f * TO_RADIAN;
            // camera_info[CAMERA_1].near_c += 0.1f;
            // camera_info[CAMERA_1].far_c += 0.1f;
            if (camera_info[CAMERA_2].fovy > 179 * TO_RADIAN)
                camera_info[CAMERA_2].fovy = 179 * TO_RADIAN;
            set_current_camera(CAMERA_2);
            glutPostRedisplay();
        }
    }
    if (c_cam == 5 && specialKey == GLUT_ACTIVE_CTRL)
    {
        if (direction > 0)
        {
            camera_info[CAMERA_5].fovy -= 5.0f * TO_RADIAN;
            // camera_info[CAMERA_1].near_c -= 0.1f;
            // camera_info[CAMERA_1].far_c -= 0.1f;
            if (camera_info[CAMERA_5].fovy < 1 * TO_RADIAN)
                camera_info[CAMERA_5].fovy = 1 * TO_RADIAN;
            set_current_camera(CAMERA_5);
            glutPostRedisplay();
        }
        else
        {
            camera_info[CAMERA_5].fovy += 5.0f * TO_RADIAN;
            // camera_info[CAMERA_1].near_c += 0.1f;
            // camera_info[CAMERA_1].far_c += 0.1f;
            if (camera_info[CAMERA_5].fovy > 179 * TO_RADIAN)
                camera_info[CAMERA_5].fovy = 179 * TO_RADIAN;
            set_current_camera(CAMERA_5);
            glutPostRedisplay();
        }
    }
    if (c_check == 0)
        return;
    if (c_cam == 6 && specialKey == GLUT_ACTIVE_CTRL)
    {
        if (direction > 0)
        {
            camera_info[CAMERA_6].fovy -= 5.0f * TO_RADIAN;
            // camera_info[CAMERA_1].near_c -= 0.1f;
            // camera_info[CAMERA_1].far_c -= 0.1f;
            if (camera_info[CAMERA_6].fovy < 1 * TO_RADIAN)
                camera_info[CAMERA_6].fovy = 1 * TO_RADIAN;
            set_current_camera(CAMERA_6);
            glutPostRedisplay();
        }
        else
        {
            camera_info[CAMERA_6].fovy += 5.0f * TO_RADIAN;
            // camera_info[CAMERA_1].near_c += 0.1f;
            // camera_info[CAMERA_1].far_c += 0.1f;
            if (camera_info[CAMERA_6].fovy > 179 * TO_RADIAN)
                camera_info[CAMERA_6].fovy = 179 * TO_RADIAN;
            set_current_camera(CAMERA_6);
            glutPostRedisplay();
        }
    }
    if (c_cam == 7 && specialKey == GLUT_ACTIVE_CTRL)
    {
        if (direction > 0)
        {
            camera_info[CAMERA_7].fovy -= 5.0f * TO_RADIAN;
            // camera_info[CAMERA_1].near_c -= 0.1f;
            // camera_info[CAMERA_1].far_c -= 0.1f;
            if (camera_info[CAMERA_7].fovy < 1 * TO_RADIAN)
                camera_info[CAMERA_7].fovy = 1 * TO_RADIAN;
            set_current_camera(CAMERA_7);
            glutPostRedisplay();
        }
        else
        {
            camera_info[CAMERA_7].fovy += 5.0f * TO_RADIAN;
            // camera_info[CAMERA_1].near_c += 0.1f;
            // camera_info[CAMERA_1].far_c += 0.1f;
            if (camera_info[CAMERA_7].fovy > 179 * TO_RADIAN)
                camera_info[CAMERA_7].fovy = 179 * TO_RADIAN;
            set_current_camera(CAMERA_7);
            glutPostRedisplay();
        }
    }
}

int drone_timer = 0;
int lr_chk = 0;
void keyboard(unsigned char key, int x, int y)
{
    glm::mat3 RotationMatrix;
    key = std::tolower(key);
    switch (key)
    {
    case 'f':
        b_draw_grid = b_draw_grid ? false : true;
        glutPostRedisplay();
        break;
    case 'a': // free_cam
        c_cam = -1;
        c_check = 1;
        lr_chk = 0;
        is_free_cam = 1;
        set_current_camera(CAMERA_1);
        glutPostRedisplay();
        break;
    case 'u': // cam1
        c_check = 1;
        lr_chk = 0;
        is_free_cam = 0;
        c_cam = 2;
        set_current_camera(CAMERA_2);
        glutPostRedisplay();
        break;
    case 't': // tiger cam
        c_cam = -1;
        c_check = 0;
        is_free_cam = 2;
        set_current_camera(CAMERA_3);
        glutPostRedisplay();
        break;
    case 'g': // tiger back cam
        c_cam = -1;
        c_check = 0;
        is_free_cam = 3;
        set_current_camera(CAMERA_3);
        glutPostRedisplay();
        break;
    case '0':
        c_cam = -1;
        c_check = 0;
        lr_chk = 0;
        set_current_camera(CAMERA_4);
        drone_timer = 0;
        camera_info[CAMERA_4].pos[0] = 5418.375977;
        camera_info[CAMERA_4].pos[1] = -3190.530762;
        camera_info[CAMERA_4].pos[2] = 2054.045166;
        camera_info[CAMERA_4].uaxis[0] = -0.145179;
        camera_info[CAMERA_4].uaxis[1] = -1.180712;
        camera_info[CAMERA_4].uaxis[2] = 0.061444;
        camera_info[CAMERA_4].vaxis[0] = -0.610642;
        camera_info[CAMERA_4].vaxis[1] = 0.172813;
        camera_info[CAMERA_4].vaxis[2] = 0.772801;
        camera_info[CAMERA_4].naxis[0] = 0.775852;
        camera_info[CAMERA_4].naxis[1] = -1.507460;
        camera_info[CAMERA_4].naxis[2] = 1.170281;
        is_free_cam = 7;
        set_current_camera(CAMERA_3);
        glutPostRedisplay();
        break;
    case 'i': // cam2
        c_check = 1;
        lr_chk = 0;
        is_free_cam = 0;
        c_cam = 5;
        set_current_camera(CAMERA_5);
        glutPostRedisplay();
        break;
    case 'o': // cam3
        c_check = 1;
        lr_chk = 0;
        is_free_cam = 0;
        c_cam = 6;
        set_current_camera(CAMERA_6);
        glutPostRedisplay();
        break;
    case 'p': // cam4
        c_check = 1;
        lr_chk = 0;
        is_free_cam = 0;
        c_cam = 7;
        set_current_camera(CAMERA_7);
        glutPostRedisplay();
        break;
    case 'q': // z-rotate +
        if (is_free_cam == 1)
        {
            RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0f), -1.25f * TO_RADIAN, camera_info[CAMERA_1].naxis));
            camera_info[CAMERA_1].uaxis = RotationMatrix * camera_info[CAMERA_1].uaxis;
            camera_info[CAMERA_1].vaxis = RotationMatrix * camera_info[CAMERA_1].vaxis;
            set_current_camera(CAMERA_1);
            glutPostRedisplay();
        }
        break;
    case 'e': // z-rotate -
        if (is_free_cam == 1)
        {
            RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0f), 1.25f * TO_RADIAN, camera_info[CAMERA_1].naxis));
            camera_info[CAMERA_1].uaxis = RotationMatrix * camera_info[CAMERA_1].uaxis;
            camera_info[CAMERA_1].vaxis = RotationMatrix * camera_info[CAMERA_1].vaxis;
            set_current_camera(CAMERA_1);
            glutPostRedisplay();
        }
        break;
    case ' ': // z+
        if (is_free_cam == 1)
        {
            camera_info[CAMERA_1].pos += (12.0f * camera_info[CAMERA_1].naxis);
            set_current_camera(CAMERA_1);
            glutPostRedisplay();
        }
        break;
    case '`': // z-
        if (is_free_cam == 1)
        {
            camera_info[CAMERA_1].pos -= (12.0f * camera_info[CAMERA_1].naxis);
            set_current_camera(CAMERA_1);
            glutPostRedisplay();
        }
        break;
    case '-': // stop
        flag_tiger_animation = 1 - flag_tiger_animation;
        glutTimerFunc(100, timer_scene, 0);
        break;
    case 'w':
        spider_info.z += 10.0f;
        // glutPostRedisplay();
        break;
    case 's':
        spider_info.z -= 10.0f;
        // glutPostRedisplay();
        break;
    case 'y': // spider eye
        c_cam = -1;
        c_check = 0;
        is_free_cam = 4;
        set_current_camera(CAMERA_3);
        break;
    case 'd':
        spider_chk = 1 - spider_chk;
        break;
    case 'h':
        c_cam = -1;
        c_check = 0;
        is_free_cam = 5;
        set_current_camera(CAMERA_3);
        break;
    case 'n':
        c_cam = -1;
        c_check = 0;
        set_current_camera(CAMERA_3);
        is_free_cam = 6;
        break;
    case ';':
        printf("-----------------------------------------------------------------------------------\n");
        printf("pos\t|\tx : %f\ty : %f\tz : %f\t\n", camera_info[CAMERA_1].pos[0], camera_info[CAMERA_1].pos[1], camera_info[CAMERA_1].pos[2]);
        printf("uaxis\t|\tx : %f\ty : %f\tz : %f\t\n", camera_info[CAMERA_1].uaxis[0], camera_info[CAMERA_1].uaxis[1], camera_info[CAMERA_1].uaxis[2]);
        printf("vaxis\t|\tx : %f\ty : %f\tz : %f\t\n", camera_info[CAMERA_1].vaxis[0], camera_info[CAMERA_1].vaxis[1], camera_info[CAMERA_1].vaxis[2]);
        printf("naxis\t|\tx : %f\ty : %f\tz : %f\t\n", camera_info[CAMERA_1].naxis[0], camera_info[CAMERA_1].naxis[1], camera_info[CAMERA_1].naxis[2]);
        printf("-----------------------------------------------------------------------------------\n");
        break;
    case 27:                 // ESC key
        glutLeaveMainLoop(); // Incur destuction callback for cleanups.
        break;
    }
}

void reshape(int width, int height)
{
    float aspect_ratio;

    glViewport(0, 0, width, height);

    ProjectionMatrix = glm::perspective(current_camera.fovy, current_camera.aspect_ratio, current_camera.near_c, current_camera.far_c);
    ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
    glutPostRedisplay();
}

// 554.873474f, 1999.583862f, 20.31f > 1506.291260f, 3841.273926f // dx : 951.417786 , dy : 1841.690064
glm::vec4 teye(0.0f, -88.0f, 62.0f, 1.0f);
glm::vec4 teye_dir(0.0f, -89.0f, 62.0f, 1.0f);
glm::vec4 back_cam(0.0f, 658.0f, 382.0f, 1.0f);
void tiger_moving(void)
{
    int moving_tiger = timestamp_scene % 1360;
    int dir_trem = timestamp_scene % 100;
    // printf("moving_tiger : %d\n", moving_tiger);

    if (moving_tiger < 600)
    {
        tiger_info.x += 1.585696f;
        tiger_info.y += 3.069483f;
    }
    else if (600 <= moving_tiger && moving_tiger < 640)
    {
        tiger_info.z += 5.5f;
        tiger_info.angle += 4.5f;
    }
    else if (640 <= moving_tiger && moving_tiger < 680)
    {
        tiger_info.z -= 5.5f;
        tiger_info.angle = 330;
    }
    else if (680 <= moving_tiger && moving_tiger < 1280)
    {
        tiger_info.x -= 1.585696f;
        tiger_info.y -= 3.069483f;
    }
    else if (1280 <= moving_tiger && moving_tiger < 1320)
    {
        tiger_info.z += 5.5f;
        tiger_info.angle -= 4.5f;
    }
    else
    {
        tiger_info.z -= 5.5f;
        tiger_info.angle = 150;
    }

    if (dir_trem < 25)
    {
        teye_dir[2] += 0.00125f;
    }
    else if (dir_trem < 75)
    {
        teye_dir[2] -= 0.00125f;
    }
    else
    {
        teye_dir[2] += 0.00125f;
    }

    glm::mat4 TransMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(tiger_info.x, tiger_info.y, tiger_info.z));
    TransMatrix = glm::rotate(TransMatrix, tiger_info.angle * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
    camera_info[CAMERA_3].pos = (glm::vec3)(TransMatrix * teye);
    glm::vec3 point = (glm::vec3)(TransMatrix * teye_dir);
    glm::vec3 back = (glm::vec3)(TransMatrix * back_cam);
    glm::mat4 new_cam = glm::lookAt(camera_info[CAMERA_3].pos, point, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 new_cam2 = glm::lookAt(back, camera_info[CAMERA_3].pos, glm::vec3(0.0f, 0.0f, 1.0f));

    if (is_free_cam == 2)
    {
        ViewMatrix = new_cam;
        glutPostRedisplay();
    }

    if (is_free_cam == 3)
    {
        ViewMatrix = new_cam2;
        glutPostRedisplay();
    }
}

void spider_moving()
{
    if (spider_chk == 0)
    {
        spider_info.x = 598.684082f;
        spider_info.y = 2873.774658f;
    }
    else
    {
        spider_info.x = 1665.853760f;
        spider_info.y = 3207.652344f;
    }
    if (spider_info.z < 153.813354)
    {
        spider_info.z = 153.813354;
    }
    if (spider_info.z > 1343.813354)
    {
        spider_info.z = 1343.813354;
    }
    glm::mat4 new_cam = glm::lookAt(glm::vec3(spider_info.x, spider_info.y, spider_info.z - 30.0f), glm::vec3(tiger_info.x, tiger_info.y, tiger_info.z), glm::vec3(0.0f, 0.0f, 1.0f));
    if (is_free_cam == 4)
    {
        ViewMatrix = new_cam;
        glutPostRedisplay();
    }
}

void ben_moving()
{
    int ben_timer = timestamp_scene % 1801;
    int ben_jump = timestamp_scene % 28;
    if (ben_timer < 600)
    {
        ben_info.x += 6.504605f;
        ben_info.y -= 2.541546f;
    }
    else if (ben_timer == 600)
    {
        ben_info.x = 3516.069092f;
        ben_info.y = -2217.735596f;
        ben_info.angle = 160;
    }
    else if (ben_timer < 750)
    {
        ben_info.x += 8.083536f;
        ben_info.y += 2.542657f;
        ben_info.angle -= 0.6f;
    }
    else if (ben_timer == 750)
    {
        ben_info.x = 4728.599609f;
        ben_info.y = -1836.337036f;
        ben_info.angle = 70;
    }
    else if (ben_timer < 900)
    {
        ben_info.x += 2.479772f;
        ben_info.y -= 7.202971f;
        ben_info.angle -= 0.6f;
    }
    else if (ben_timer == 900)
    {
        ben_info.x = 5100.565430f;
        ben_info.y = -2916.782712f;
        ben_info.angle = -20;
    }
    else if (ben_timer < 1050)
    {
        ben_info.x -= 6.985f;
        ben_info.y -= 1.762148f;
        ben_info.angle -= 0.6f;
    }
    else if (ben_timer == 1050)
    {
        ben_info.x = 4052.815430f;
        ben_info.y = -3181.104980f;
        ben_info.angle = -110;
    }
    else if (ben_timer < 1200)
    {
        ben_info.x -= 3.578309f;
        ben_info.y += 6.422462f;
        ben_info.angle -= 0.6;
    }
    else if (ben_timer == 1200)
    { // ben_info.angle = -200;
        ben_info.x = 3516.069092f;
        ben_info.y = -2217.735596f;
        ben_info.angle = -110;
    }
    else if (ben_timer < 1800)
    {
        ben_info.x -= 6.504605f;
        ben_info.y += 2.541546f;
    }
    else if (ben_timer == 1800)
    {
        ben_info.x = -386.693970f;
        ben_info.y = -692.807983f;
        ben_info.angle = 70;
    }

    if (ben_jump < 14)
    {
        ben_info.z += 2.0f;
    }
    else
    {
        ben_info.z -= 2.0f;
    }

    glm::mat4 new_cam2 = glm::lookAt(glm::vec3(3581.419922f, -2132.625000f, 2830.813721f), glm::vec3(ben_info.x, ben_info.y, 25.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 new_cam = glm::lookAt(glm::vec3(ben_info.x - 900.0f, ben_info.y + 400.0f, ben_info.z + 600.0f), glm::vec3(ben_info.x, ben_info.y, ben_info.z), glm::vec3(0.0f, 0.0f, 1.0f));
    if (is_free_cam == 5)
    {
        ViewMatrix = new_cam;
        glutPostRedisplay();
    }
    if (is_free_cam == 6)
    {
        ViewMatrix = new_cam2;
        glutPostRedisplay();
    }
}

void drone()
{
    if (is_free_cam == 7)
    {
        drone_timer++;
        drone_timer = drone_timer % 4501;
    }

    if (drone_timer < 500)
    {
        camera_info[CAMERA_4].pos[0] += (3860.095215 - 5418.375977) / 500;
        camera_info[CAMERA_4].pos[1] += (-1894.932373 + 3190.530762) / 500;
        camera_info[CAMERA_4].pos[2] += (1396.862793 - 2054.045166) / 500;
        camera_info[CAMERA_4].uaxis[0] += (0.275385 + 0.145179) / 500;
        camera_info[CAMERA_4].uaxis[1] += (-1.157522 + 1.180712) / 500;
        camera_info[CAMERA_4].uaxis[2] += (0.056845 - 0.061444) / 500;
        camera_info[CAMERA_4].vaxis[0] += (-0.255662 + 0.610642) / 500;
        camera_info[CAMERA_4].vaxis[1] += (0.045264 - 0.172813) / 500;
        camera_info[CAMERA_4].vaxis[2] += (0.965689 - 0.772801) / 500;
        camera_info[CAMERA_4].naxis[0] += (1.624102 - 0.775852) / 500;
        camera_info[CAMERA_4].naxis[1] += (-1.083816 + 1.507460) / 500;
        camera_info[CAMERA_4].naxis[2] += (0.656937 - 1.170281) / 500;
    }
    else if (drone_timer == 500)
    {
        camera_info[CAMERA_4].pos[0] = 3860.095215;
        camera_info[CAMERA_4].pos[1] = -1894.932373;
        camera_info[CAMERA_4].pos[2] = 1396.862793;
        camera_info[CAMERA_4].uaxis[0] = 0.275385;
        camera_info[CAMERA_4].uaxis[1] = -1.157522;
        camera_info[CAMERA_4].uaxis[2] = 0.056845;
        camera_info[CAMERA_4].vaxis[0] = -0.255662;
        camera_info[CAMERA_4].vaxis[1] = 0.045264;
        camera_info[CAMERA_4].vaxis[2] = 0.965689;
        camera_info[CAMERA_4].naxis[0] = 1.624102;
        camera_info[CAMERA_4].naxis[1] = -1.083816;
        camera_info[CAMERA_4].naxis[2] = 0.656937;
    }
    else if (drone_timer < 1000)
    {
        camera_info[CAMERA_4].pos[0] += (3097.153564 - 3860.095215) / 500;
        camera_info[CAMERA_4].pos[1] += (-1489.899170 + 1894.932373) / 500;
        camera_info[CAMERA_4].pos[2] += (976.169373 - 1396.862793) / 500;
        camera_info[CAMERA_4].uaxis[0] += (0.609885 - 0.275385) / 500;
        camera_info[CAMERA_4].uaxis[1] += (-1.020784 + 1.157522) / 500;
        camera_info[CAMERA_4].uaxis[2] += (0.070460 - 0.056845) / 500;
        camera_info[CAMERA_4].vaxis[0] += (-0.072116 + 0.255662) / 500;
        camera_info[CAMERA_4].vaxis[1] += (0.091987 - 0.045264) / 500;
        camera_info[CAMERA_4].vaxis[2] += (0.993129 - 0.965689) / 500;
        camera_info[CAMERA_4].naxis[0] += (1.964324 - 1.624102) / 500;
        camera_info[CAMERA_4].naxis[1] += (-0.505209 + 1.083816) / 500;
        camera_info[CAMERA_4].naxis[2] += (0.360728 - 0.656937) / 500;
    }
    else if (drone_timer == 1000)
    {
        camera_info[CAMERA_4].pos[0] = 3097.153564;
        camera_info[CAMERA_4].pos[1] = -1489.899170;
        camera_info[CAMERA_4].pos[2] = 976.169373;
        camera_info[CAMERA_4].uaxis[0] = 0.609885;
        camera_info[CAMERA_4].uaxis[1] = -1.020784;
        camera_info[CAMERA_4].uaxis[2] = 0.070460;
        camera_info[CAMERA_4].vaxis[0] = -0.072116;
        camera_info[CAMERA_4].vaxis[1] = 0.091987;
        camera_info[CAMERA_4].vaxis[2] = 0.993129;
        camera_info[CAMERA_4].naxis[0] = 1.964324;
        camera_info[CAMERA_4].naxis[1] = -0.505209;
        camera_info[CAMERA_4].naxis[2] = 0.360728;
    }
    else if (drone_timer < 1500)
    {
        camera_info[CAMERA_4].pos[0] += (2163.500732 - 3097.153564) / 500;
        camera_info[CAMERA_4].pos[1] += (-1445.489746 + 1489.899170) / 500;
        camera_info[CAMERA_4].pos[2] += (493.566345 - 976.169373) / 500;
        camera_info[CAMERA_4].uaxis[0] += (0.604458 - 0.609885) / 500;
        camera_info[CAMERA_4].uaxis[1] += (-1.025262 + 1.020784) / 500;
        camera_info[CAMERA_4].uaxis[2] += (0.048880 - 0.070460) / 500;
        camera_info[CAMERA_4].vaxis[0] += (-0.073457 + 0.072116) / 500;
        camera_info[CAMERA_4].vaxis[1] += (0.070351 - 0.091987) / 500;
        camera_info[CAMERA_4].vaxis[2] += (0.994797 - 0.993129) / 500;
        camera_info[CAMERA_4].naxis[0] += (1.961013 - 1.964324) / 500;
        camera_info[CAMERA_4].naxis[1] += (-0.523317 + 0.505209) / 500;
        camera_info[CAMERA_4].naxis[2] += (0.352820 - 0.360728) / 500;
    }
    else if (drone_timer == 1500)
    {
        camera_info[CAMERA_4].pos[0] = 2163.500732;
        camera_info[CAMERA_4].pos[1] = -1445.489746;
        camera_info[CAMERA_4].pos[2] = 493.566345;
        camera_info[CAMERA_4].uaxis[0] = 0.604458;
        camera_info[CAMERA_4].uaxis[1] = -1.025262;
        camera_info[CAMERA_4].uaxis[2] = 0.048880;
        camera_info[CAMERA_4].vaxis[0] = -0.073457;
        camera_info[CAMERA_4].vaxis[1] = 0.070351;
        camera_info[CAMERA_4].vaxis[2] = 0.994797;
        camera_info[CAMERA_4].naxis[0] = 1.961013;
        camera_info[CAMERA_4].naxis[1] = -0.523317;
        camera_info[CAMERA_4].naxis[2] = 0.352820;
    }
    else if (drone_timer < 2000)
    {
        camera_info[CAMERA_4].pos[0] += (375.058594 - 2163.500732) / 500;
        camera_info[CAMERA_4].pos[1] += (-968.226074 + 1445.489746) / 500;
        camera_info[CAMERA_4].pos[2] += (171.795288 - 493.566345) / 500;
        camera_info[CAMERA_4].uaxis[0] += (-0.018596 - 0.604458) / 500;
        camera_info[CAMERA_4].uaxis[1] += (-1.190949 + 1.025262) / 500;
        camera_info[CAMERA_4].uaxis[2] += (0.014590 - 0.04888) / 500;
        camera_info[CAMERA_4].vaxis[0] += (-0.073457 + 0.073457) / 500;
        camera_info[CAMERA_4].vaxis[1] += (0.070351 - 0.070351) / 500;
        camera_info[CAMERA_4].vaxis[2] += (0.994797 - 0.994797) / 500;
        camera_info[CAMERA_4].naxis[0] += (1.385211 - 1.961013) / 500;
        camera_info[CAMERA_4].naxis[1] += (-1.477290 + 0.523317) / 500;
        camera_info[CAMERA_4].naxis[2] += (0.377766 - 0.352820) / 500;
    }
    else if (drone_timer == 2000)
    {
        camera_info[CAMERA_4].pos[0] = 375.058594;
        camera_info[CAMERA_4].pos[1] = -968.226074;
        camera_info[CAMERA_4].pos[2] = 171.795288;
        camera_info[CAMERA_4].uaxis[0] = -0.018596;
        camera_info[CAMERA_4].uaxis[1] = -1.190949;
        camera_info[CAMERA_4].uaxis[2] = 0.014590;
        camera_info[CAMERA_4].vaxis[0] = -0.073457;
        camera_info[CAMERA_4].vaxis[1] = 0.070351;
        camera_info[CAMERA_4].vaxis[2] = 0.994797;
        camera_info[CAMERA_4].naxis[0] = 1.385211;
        camera_info[CAMERA_4].naxis[1] = -1.477290;
        camera_info[CAMERA_4].naxis[2] = 0.377766;
    }
    else if (drone_timer < 2500)
    {
        camera_info[CAMERA_4].pos[0] += (-291.952362 - 375.058594) / 500;
        camera_info[CAMERA_4].pos[1] += (-272.594299 + 968.226074) / 500;
        camera_info[CAMERA_4].pos[2] += (352.056580 - 171.795288) / 500;
        camera_info[CAMERA_4].uaxis[0] += (-0.696468 + 0.018596) / 500;
        camera_info[CAMERA_4].uaxis[1] += (-0.955589 + 1.190949) / 500;
        camera_info[CAMERA_4].uaxis[2] += (0.143879 - 0.014590) / 500;
        camera_info[CAMERA_4].vaxis[0] += (0.107842 + 0.073457) / 500;
        camera_info[CAMERA_4].vaxis[1] += (0.140641 - 0.070351) / 500;
        camera_info[CAMERA_4].vaxis[2] += (0.984152 - 0.994797) / 500;
        camera_info[CAMERA_4].naxis[0] += (0.321327 - 1.385211) / 500;
        camera_info[CAMERA_4].naxis[1] += (-1.990594 + 1.477290) / 500;
        camera_info[CAMERA_4].naxis[2] += (0.422113 - 0.377766) / 500;
    }
    else if (drone_timer == 2500)
    {
        camera_info[CAMERA_4].pos[0] = -291.952362;
        camera_info[CAMERA_4].pos[1] = -272.594299;
        camera_info[CAMERA_4].pos[2] = 352.056580;
        camera_info[CAMERA_4].uaxis[0] = -0.696468;
        camera_info[CAMERA_4].uaxis[1] = -0.955589;
        camera_info[CAMERA_4].uaxis[2] = 0.143879;
        camera_info[CAMERA_4].vaxis[0] = 0.107842;
        camera_info[CAMERA_4].vaxis[1] = 0.140641;
        camera_info[CAMERA_4].vaxis[2] = 0.984152;
        camera_info[CAMERA_4].naxis[0] = 0.321327;
        camera_info[CAMERA_4].naxis[1] = -1.990594;
        camera_info[CAMERA_4].naxis[2] = 0.422113;
    }
    else if (drone_timer < 3000)
    {
        camera_info[CAMERA_4].pos[0] += (-450.798340 + 291.952362) / 500;
        camera_info[CAMERA_4].pos[1] += (580.503418 + 272.594299) / 500;
        camera_info[CAMERA_4].pos[2] += (176.330307 - 352.056580) / 500;
        camera_info[CAMERA_4].uaxis[0] += (-1.169256 + 0.696468) / 500;
        camera_info[CAMERA_4].uaxis[1] += (0.225920 + 0.955589) / 500;
        camera_info[CAMERA_4].uaxis[2] += (0.026841 - 0.143879) / 500;
        camera_info[CAMERA_4].vaxis[0] += (0.068560 - 0.107842) / 500;
        camera_info[CAMERA_4].vaxis[1] += (-0.064024 - 0.140641) / 500;
        camera_info[CAMERA_4].vaxis[2] += (0.995572 - 0.984152) / 500;
        camera_info[CAMERA_4].naxis[0] += (-1.702645 - 0.321327) / 500;
        camera_info[CAMERA_4].naxis[1] += (-1.139626 + 1.990594) / 500;
        camera_info[CAMERA_4].naxis[2] += (0.214837 - 0.422113) / 500;
    }
    else if (drone_timer == 3000)
    {
        camera_info[CAMERA_4].pos[0] = -450.798340;
        camera_info[CAMERA_4].pos[1] = 580.503418;
        camera_info[CAMERA_4].pos[2] = 176.330307;
        camera_info[CAMERA_4].uaxis[0] = -1.169256;
        camera_info[CAMERA_4].uaxis[1] = 0.225920;
        camera_info[CAMERA_4].uaxis[2] = 0.026841;
        camera_info[CAMERA_4].vaxis[0] = 0.068560;
        camera_info[CAMERA_4].vaxis[1] = -0.064024;
        camera_info[CAMERA_4].vaxis[2] = 0.995572;
        camera_info[CAMERA_4].naxis[0] = -1.702645;
        camera_info[CAMERA_4].naxis[1] = -1.139626;
        camera_info[CAMERA_4].naxis[2] = 0.214837;
    }
    else if (drone_timer < 3500)
    {
        camera_info[CAMERA_4].pos[0] += (730.143677 + 450.798340) / 500;
        camera_info[CAMERA_4].pos[1] += (2161.853760 - 580.503418) / 500;
        camera_info[CAMERA_4].pos[2] += (486.922180 - 176.330307) / 500;
        camera_info[CAMERA_4].uaxis[0] += (-1.174960 + 1.169256) / 500;
        camera_info[CAMERA_4].uaxis[1] += (-0.192578 - 0.225920) / 500;
        camera_info[CAMERA_4].uaxis[2] += (-0.036063 - 0.026841) / 500;
        camera_info[CAMERA_4].vaxis[0] += (0.088966 - 0.068560) / 500;
        camera_info[CAMERA_4].vaxis[1] += (-0.363816 + 0.064024) / 500;
        camera_info[CAMERA_4].vaxis[2] += (0.927193 - 0.995572) / 500;
        camera_info[CAMERA_4].naxis[0] += (-1.201143 + 1.702645) / 500;
        camera_info[CAMERA_4].naxis[1] += (-1.637938 + 1.139626) / 500;
        camera_info[CAMERA_4].naxis[2] += (-0.343975 - 0.214837) / 500;
    }
    else if (drone_timer == 3500)
    {
        camera_info[CAMERA_4].pos[0] = 730.143677;
        camera_info[CAMERA_4].pos[1] = 2161.853760;
        camera_info[CAMERA_4].pos[2] = 486.922180;
        camera_info[CAMERA_4].uaxis[0] = -1.174960;
        camera_info[CAMERA_4].uaxis[1] = -0.192578;
        camera_info[CAMERA_4].uaxis[2] = -0.036063;
        camera_info[CAMERA_4].vaxis[0] = 0.088966;
        camera_info[CAMERA_4].vaxis[1] = -0.363816;
        camera_info[CAMERA_4].vaxis[2] = 0.927193;
        camera_info[CAMERA_4].naxis[0] = -1.201143;
        camera_info[CAMERA_4].naxis[1] = -1.637938;
        camera_info[CAMERA_4].naxis[2] = -0.343975;
    }
    else if (drone_timer < 4000)
    {
        camera_info[CAMERA_4].pos[0] += (1629.200928 - 730.143677) / 500;
        camera_info[CAMERA_4].pos[1] += (3921.083496 - 2161.853760) / 500;
        camera_info[CAMERA_4].pos[2] += (917.106445 - 486.922180) / 500;
        camera_info[CAMERA_4].uaxis[0] += (-0.954377 + 1.174960) / 500;
        camera_info[CAMERA_4].uaxis[1] += (-0.669591 + 0.192578) / 500;
        camera_info[CAMERA_4].uaxis[2] += (-0.244400 + 0.036063) / 500;
        camera_info[CAMERA_4].vaxis[0] += (0.261281 - 0.088966) / 500;
        camera_info[CAMERA_4].vaxis[1] += (-0.558375 + 0.363816) / 500;
        camera_info[CAMERA_4].vaxis[2] += (0.787346 - 0.927193) / 500;
        camera_info[CAMERA_4].naxis[0] += (-0.316330 + 1.201143) / 500;
        camera_info[CAMERA_4].naxis[1] += (-1.798155 + 1.637938) / 500;
        camera_info[CAMERA_4].naxis[2] += (-0.954191 + 0.343975) / 500;
    }
    else if (drone_timer == 4000)
    {
        camera_info[CAMERA_4].pos[0] = 1629.200928;
        camera_info[CAMERA_4].pos[1] = 3921.083496;
        camera_info[CAMERA_4].pos[2] = 917.106445;
        camera_info[CAMERA_4].uaxis[0] = -0.954377;
        camera_info[CAMERA_4].uaxis[1] = -0.669591;
        camera_info[CAMERA_4].uaxis[2] = -0.244400;
        camera_info[CAMERA_4].vaxis[0] = 0.261281;
        camera_info[CAMERA_4].vaxis[1] = -0.558375;
        camera_info[CAMERA_4].vaxis[2] = 0.787346;
        camera_info[CAMERA_4].naxis[0] = -0.316330;
        camera_info[CAMERA_4].naxis[1] = -1.798155;
        camera_info[CAMERA_4].naxis[2] = -0.954191;
    }
    else if (drone_timer < 4500)
    {
        camera_info[CAMERA_4].pos[0] += (5418.375977 - 1629.200928) / 500;
        camera_info[CAMERA_4].pos[1] += (-3190.530762 - 3921.083496) / 500;
        camera_info[CAMERA_4].pos[2] += (2054.045166 - 917.106445) / 500;
        camera_info[CAMERA_4].uaxis[0] += (-0.145179 + 0.954377) / 500;
        camera_info[CAMERA_4].uaxis[1] += (-1.180712 + 0.669591) / 500;
        camera_info[CAMERA_4].uaxis[2] += (0.061444 + 0.244400) / 500;
        camera_info[CAMERA_4].vaxis[0] += (-0.610642 - 0.261281) / 500;
        camera_info[CAMERA_4].vaxis[1] += (0.172813 + 0.558375) / 500;
        camera_info[CAMERA_4].vaxis[2] += (0.772801 - 0.787346) / 500;
        camera_info[CAMERA_4].naxis[0] += (0.775852 + 0.316330) / 500;
        camera_info[CAMERA_4].naxis[1] += (-1.507460 + 1.798155) / 500;
        camera_info[CAMERA_4].naxis[2] += (1.170281 + 0.954191) / 500;
    }
    else if (drone_timer == 4500)
    {
        camera_info[CAMERA_4].pos[0] = 5418.375977;
        camera_info[CAMERA_4].pos[1] = -3190.530762;
        camera_info[CAMERA_4].pos[2] = 2054.045166;
        camera_info[CAMERA_4].uaxis[0] = -0.145179;
        camera_info[CAMERA_4].uaxis[1] = -1.180712;
        camera_info[CAMERA_4].uaxis[2] = 0.061444;
        camera_info[CAMERA_4].vaxis[0] = -0.610642;
        camera_info[CAMERA_4].vaxis[1] = 0.172813;
        camera_info[CAMERA_4].vaxis[2] = 0.772801;
        camera_info[CAMERA_4].naxis[0] = 0.775852;
        camera_info[CAMERA_4].naxis[1] = -1.507460;
        camera_info[CAMERA_4].naxis[2] = 1.170281;
    }

    if (is_free_cam == 7)
    {
        set_current_camera(CAMERA_4);
        glutPostRedisplay();
    }
}

void timer_scene(int value)
{
    timestamp_scene = (timestamp_scene + 1) % UINT_MAX;
    cur_frame_tiger = (timestamp_scene / 10) % N_TIGER_FRAMES;
    cur_frame_spider = (timestamp_scene / 3) % N_SPIDER_FRAMES;
    cur_frame_ben = (timestamp_scene / 2) % N_BEN_FRAMES;
    glutPostRedisplay();
    if (flag_tiger_animation)
    {
        glutTimerFunc(10, timer_scene, 0);
        tiger_moving();
        spider_moving();
        ben_moving();
        drone();
    }
}

void cleanup(void)
{
    glDeleteVertexArrays(1, &axes_VAO);
    glDeleteBuffers(1, &axes_VBO);

    glDeleteVertexArrays(1, &grid_VAO);
    glDeleteBuffers(1, &grid_VBO);

    glDeleteVertexArrays(scene.n_materials, bistro_exterior_VAO);
    glDeleteBuffers(scene.n_materials, bistro_exterior_VBO);
    glDeleteTextures(scene.n_textures, bistro_exterior_texture_names);

    glDeleteVertexArrays(1, &skybox_VAO);
    glDeleteBuffers(1, &skybox_VBO);

    free(bistro_exterior_n_triangles);
    free(bistro_exterior_vertex_offset);

    free(bistro_exterior_VAO);
    free(bistro_exterior_VBO);

    free(bistro_exterior_texture_names);
    free(flag_texture_mapping);
}
/*********************  END: callback function definitions **********************/

void register_callbacks(void)
{
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutReshapeFunc(reshape);
    glutCloseFunc(cleanup);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutMouseWheelFunc(wheel);
    glutTimerFunc(100, timer_scene, 0);
}

void initialize_OpenGL(void)
{
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    ViewMatrix = glm::mat4(1.0f);
    ProjectionMatrix = glm::mat4(1.0f);
    ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;

    initialize_lights();
}

void prepare_scene(void)
{
    prepare_axes();
    prepare_grid();
    prepare_bistro_exterior();
    prepare_skybox();

    prepare_tiger();
    prepare_spider();
    prepare_ben();
    prepare_ironman();
    prepare_cow();
    prepare_bike();
    prepare_godzilla();
    prepare_bus();
}

void initialize_renderer(void)
{
    register_callbacks();
    prepare_shader_program();
    initialize_OpenGL();
    prepare_scene();
    initialize_camera();
}

void initialize_glew(void)
{
    GLenum error;

    glewExperimental = GL_TRUE;

    error = glewInit();
    if (error != GLEW_OK)
    {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(error));
        exit(-1);
    }
    fprintf(stdout, "********************************************************************************\n");
    fprintf(stdout, " - GLEW version supported: %s\n", glewGetString(GLEW_VERSION));
    fprintf(stdout, " - OpenGL renderer: %s\n", glGetString(GL_RENDERER));
    fprintf(stdout, " - OpenGL version supported: %s\n", glGetString(GL_VERSION));
    fprintf(stdout, "********************************************************************************\n\n");
}

void print_message(const char *m)
{
    fprintf(stdout, "%s\n\n", m);
}

void greetings(char *program_name, char messages[][256], int n_message_lines)
{
    fprintf(stdout, "********************************************************************************\n\n");
    fprintf(stdout, "  PROGRAM NAME: %s\n\n", program_name);
    fprintf(stdout, "    This program was coded for CSE4170 students\n");
    fprintf(stdout, "      of Dept. of Comp. Sci. & Eng., Sogang University.\n\n");

    for (int i = 0; i < n_message_lines; i++)
        fprintf(stdout, "%s\n", messages[i]);
    fprintf(stdout, "\n********************************************************************************\n\n");

    initialize_glew();
}

#define N_MESSAGE_LINES 9
void drawScene(int argc, char *argv[])
{
    char program_name[64] = "Sogang CSE4170 Bistro Exterior Scene";
    char messages[N_MESSAGE_LINES][256] = {
        "    - Keys used:",
        "		'f' : draw x, y, z axes and grid",
        "		'1' : set the camera for original view",
        "		'2' : set the camera for bistro view",
        "		'3' : set the camera for tree view",
        "		'4' : set the camera for top view",
        "		'5' : set the camera for front view",
        "		'6' : set the camera for side view",
        "		'ESC' : program close",
    };

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(900, 600);
    glutInitWindowPosition(20, 20);
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutCreateWindow(program_name);

    greetings(program_name, messages, N_MESSAGE_LINES);
    initialize_renderer();

    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutMainLoop();
}
