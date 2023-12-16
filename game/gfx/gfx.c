#include "../def.h"

#include "../gfx.h"
#include "../sys.h"
#include "../exmath.h"
#include "../game.h"

#define STBI_MALLOC(sz)           sys_malloc(sz)
#define STBI_REALLOC(p,newsz)     sys_realloc(p,newsz)
#define STBI_FREE(p)              sys_free(p)
#define STB_IMAGE_IMPLEMENTATION
#include "..\other\stb_image.h"

#define GL_EXT_MACRO(x, caps) PFN##caps##PROC x;
#include "..\gl_extensions.h"

typedef struct asset {
    textureid_t tx;
} asset_t;

shader_t shader;
hashtable_t assets;

void font_create(font_t* f, textureid_t tx, int start_letter, int row_len, int col_len, vec3f letter_size)
{
    letter_size.z = 1.f;

    f->tx = tx;
    f->start_letter = start_letter;
    f->row_len = row_len;
    f->letter_size = letter_size;
    f->col_len = col_len;

    mat4_scaling(&f->modelmat, letter_size);
    mat4_scaling(&f->texmat, VEC3F(1.f / f->row_len, 1.f / f->col_len, 1.f));
}

void gfx_do_opengl_stuff() {
    static const char* vertex_src =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec2 aTexCoord;\n"
        "layout (location = 2) in vec4 vColor;\n"
        "uniform mat4 modelMat;\n"
        "uniform mat4 viewMat;\n"
        "uniform mat4 projMat;\n"
        "uniform vec4 u_color;\n"
        "out vec4 f_color;\n"
        "out vec2 TexCoord;\n"
        "out vec4 color;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = projMat * viewMat * modelMat * vec4(aPos, 1);\n"
        "    f_color = u_color * vColor;\n"
        "    TexCoord = aTexCoord;\n"
        "}\n";
    
    static const char* frag_src =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform mat4 texTransformMat;\n"
        "uniform sampler2D tex;\n"
        "in vec4 f_color;\n"
        "in vec2 TexCoord;\n"
        "void main()\n"
        "{\n"
        "    vec4 col = texture(tex, (texTransformMat * vec4(TexCoord, 0.f, 1.f)).xy);\n"
        "    if (col.a < 0.1) discard;\n"
        "    else FragColor = f_color * col;\n"
        "}\n";
    
    GLuint vertex, frag;
    GLint success;
    static char infoLog[512];

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); 

    #define GL_EXT_MACRO(x, caps) x = (PFN##caps##PROC)wglGetProcAddress(#x); if (x == NULL) printf("%s = %p\n", #x, x);
    #include "..\gl_extensions.h"

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_src, NULL);
    glCompileShader(vertex);

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        printf("Vertex shader error: %s\n", infoLog);
    }

    frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &frag_src, NULL);
    glCompileShader(frag); 

    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(frag, 512, NULL, infoLog);
        printf("Fragment shader error: %s\n", infoLog);
    }

    shader.program = glCreateProgram();
    glAttachShader(shader.program, vertex);
    glAttachShader(shader.program, frag);
    glLinkProgram(shader.program);
    glUseProgram(shader.program);

    shader.model_mat_location           = glGetUniformLocation(shader.program, "modelMat");
    shader.view_mat_location            = glGetUniformLocation(shader.program, "viewMat");
    shader.proj_mat_location            = glGetUniformLocation(shader.program, "projMat");
    shader.color_location               = glGetUniformLocation(shader.program, "u_color");
    shader.textransform_mat_location    = glGetUniformLocation(shader.program, "texTransformMat");

    glDeleteShader(vertex);
    glDeleteShader(frag); 

    glvertex_t quad_vertices[5] = {
        { .pos = { 0.f, 0.f, 0.f }, .uv = { 0.f, 0.f }, .color = { 1.f, 1.f, 1.f, 1.f } },
        { .pos = { 1.f, 0.f, 0.f }, .uv = { 1.f, 0.f }, .color = { 1.f, 1.f, 1.f, 1.f } },
        { .pos = { 1.f, 1.f, 0.f }, .uv = { 1.f, 1.f }, .color = { 1.f, 1.f, 1.f, 1.f } },
        { .pos = { 0.f, 1.f, 0.f }, .uv = { 0.f, 1.f }, .color = { 1.f, 1.f, 1.f, 1.f } },
        { .pos = { 0.f, 0.f, 0.f }, .uv = { 0.f, 1.f }, .color = { 1.f, 1.f, 1.f, 1.f } }
    };

    glwrapGenBuffers(1, &shader.quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, shader.quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    glwrapGenVertexArrays(1, &shader.quad_vao);
    glBindVertexArray(shader.quad_vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glvertex_t), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glvertex_t), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glvertex_t), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glwrapGenTextures(1, &shader.white_texture);
    glBindTexture(GL_TEXTURE_2D, shader.white_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    static uint8_t white_data[4] = { 255, 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0);
}

vec2f font_measure_text(font_t* f, const char* text)
{
    uint32_t best_len = 0;
    uint32_t len = 0;
    uint32_t lines = 1;
    
    while (*text) {
        if (*text == '\n') {
            best_len = max(best_len, len);
            lines++;
            len = -1;
        }

        text++;
        len++;
    }

    best_len = max(best_len, len);
    return VEC2F((f->letter_size.x + 2.f) * best_len, (f->letter_size.y + 2.f) * lines);
}

void gfx_init()
{
    gfx_do_opengl_stuff();
    assets = hashtable_of(asset_t);
}

void gfx_deinit()
{
    glwrapDeleteBuffers(1, &shader.quad_vao);
    glwrapDeleteVertexArrays(1, &shader.quad_vbo);

	for (int i = 0; i < assets.n_buckets * HT_SECTION_LEN; i++) {
		if (hashtable_pick_bucket(&assets, i)->used)
            glwrapDeleteTextures(1, &HASHBUCKET_DATA(hashtable_pick_bucket(&assets, i), asset_t).tx);
    }

    glDeleteProgram(shader.program);
    hashtable_destroy(&assets);
}

void gfx_draw_2d_texture(textureid_t tx, float x, float y, float sx, float sy) {
    mat4 ident = MAT4_IDENTITY;
    mat4 modelmat;

    mat4_translation(&modelmat, VEC3F(x, y, 0.f));
    mat4_scale(&modelmat, VEC3F(sx, sy, 1));

    glUniformMatrix4fv(shader.model_mat_location, 1, GL_TRUE, modelmat.v);
    glUniformMatrix4fv(shader.textransform_mat_location, 1, GL_TRUE, ident.v);

    glBindTexture(GL_TEXTURE_2D, tx);
    glBindVertexArray(shader.quad_vao);
    glDrawArrays(GL_QUADS, 0, 4);
}

void gfx_draw_2d_texture_rect(textureid_t tx, float x, float y, float sx, float sy, float txx, float txy, float txscalex, float txscaley) {
    mat4 modelmat;
    mat4 texmat;
    
    mat4_translation(&modelmat, VEC3F(x, y, 0));
    mat4_scale(&modelmat, VEC3F(sx, sy, 1));
    mat4_translation(&texmat, VEC3F(txx, txy, 0.f));
    mat4_scale(&texmat, VEC3F(txscalex, txscaley, 1.f));

    glUniformMatrix4fv(shader.model_mat_location, 1, GL_TRUE, modelmat.v);
    glUniformMatrix4fv(shader.textransform_mat_location, 1, GL_TRUE, texmat.v);

    glBindTexture(GL_TEXTURE_2D, tx);
    glBindVertexArray(shader.quad_vao);
    glDrawArrays(GL_QUADS, 0, 4);
}

unsigned int gfx_cache_texture(char *name, unsigned int filter)
{
    textureid_t tx;
    hashbucket_t *find = hashtable_find(&assets, name);

    if (find != NULL)
        return HASHBUCKET_DATA(find, asset_t).tx;

    tx = gfx_load_texture(name, filter);
    if (tx != 0)
        HASHBUCKET_DATA(hashtable_emplace(&assets, name), asset_t).tx = tx;
    
	return tx;
}

void gfx_uncache_texture(char *name)
{
    hashbucket_t* find = hashtable_find(&assets, name);
	if (find == NULL)
        return;

    glwrapDeleteTextures(1, &HASHBUCKET_DATA(find, asset_t).tx);
    hashbucket_empty(find);
}

unsigned int gfx_load_texture(char *path, unsigned int filter)
{
    GLuint texture = 0;
    uint32_t width, height, nrChannels;

    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 4);
    
    if (data) {
        glwrapGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        if (filter == TEXTURE_LINEAR_FILTER) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);    
        }
    
        glTexImage2D(GL_TEXTURE_2D, 0, nrChannels == 3 ? GL_RGB : GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        sys_free(data);
    }

    return texture;
}

void gfx_setup_xy_screen_matrices()
{
    mat4 ident = MAT4_IDENTITY;
    mat4 projmat;

    mat4_translation(&projmat, VEC3F(-1.f, 1.f, 0.f));
    mat4_scale(&projmat, VEC3F(2.f / sys.width, -2.f / sys.height, 1.f));

    glUniform4f(shader.color_location, 1.f, 1.f, 1.f, 1.f);
    glUniformMatrix4fv(shader.view_mat_location, 1, GL_TRUE, ident.v);
    glUniformMatrix4fv(shader.proj_mat_location, 1, GL_TRUE, projmat.v);
}

void gfx_draw_text(char *str, font_t *font, vec3f pos, vec3f color)
{
    vec3f curpos = pos;

    int xs = (int)font->letter_size.x + 2;
    int ys = (int)font->letter_size.y + 2;

    glBindTexture(GL_TEXTURE_2D, font->tx);
    glBindVertexArray(shader.quad_vao);
    glUniform4f(shader.color_location, color.x, color.y, color.z, 1.f);

    while(*str) {
        int c = *str - font->start_letter;
        int x = c % font->row_len;
        int y = c / font->row_len;

        mat4 modelmat, texmat, tmpmat;
        mat4_translation(&tmpmat, curpos);
        mat4_mul(&modelmat, &tmpmat, &font->modelmat);
        mat4_translation(&tmpmat,   VEC3F((float)x / font->row_len, (float)y / font->col_len, 0.f));
        mat4_mul(&texmat, &tmpmat, &font->texmat);
        
        glUniformMatrix4fv(shader.model_mat_location, 1, GL_TRUE, modelmat.v);
        glUniformMatrix4fv(shader.textransform_mat_location, 1, GL_TRUE, texmat.v);

        glDrawArrays(GL_QUADS, 0, 4);

        if (*str == '\n') {
            curpos.x = pos.x;
            curpos.y += ys;
        }
        else if (*str == '\t')
            curpos.x += xs * 4;
        else
            curpos.x += xs;

        str++;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glUniform4f(shader.color_location, 1.f, 1.f, 1.f, 1.f);
}