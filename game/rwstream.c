#include "rwstream.h"
#include "gfx.h"
#include "utils.h"

unsigned int cur_geometry_index;
unsigned int cur_material_index;
unsigned int cur_texture_index;
unsigned int next_atomic_index;
int rw_task;
int rw_framename_index;
int rdepth;

typedef struct rwclump {
    unsigned int num_atomics;
    unsigned int num_lights;
    unsigned int num_cameras;
} rwclump_t;

typedef struct rwsection {
    unsigned int code;
    unsigned int size;
    unsigned int stamp;
} rwsection_t;

enum {
    RW_READING_NOTHING = 0,
    RW_READING_TX
};

#ifdef RWSTREAM_DEBUG
#define RW_PRINTF(fmt, ...) printf(fmt, __VA_ARGS__)
#define RW_PRINT(str) printf(str)
#else
#define RW_PRINTF(fmt, ...)
#define RW_PRINT(str)
#endif

void geometry_destroy(geometry_t* g) {
    glwrapDeleteBuffers(1, &g->vbo);
    glwrapDeleteVertexArrays(1, &g->vao);

    for (unsigned int i = 0; i < g->n_draws; i++)
        glwrapDeleteBuffers(1, &g->draws[i].ebo);

    sys_free(g->draws);
    sys_free(g->materials);
}

void dff_destroy(dff_t* dff)
{
    for (unsigned int i = 0; i < dff->n_geometries; i++) geometry_destroy(&dff->geometries[i]);
    for (unsigned int i = 0; i < dff->n_frames; i++) sys_free(dff->frames[i].name);

    sys_free(dff->atomics);
    sys_free(dff->geometries);
    sys_free(dff->frames);

    RENG_LOGF("DFF 0x%X Destroyed", dff);
}

void dff_read_entry(dff_t* dst, FILE *s);

void dff_create_from_file(dff_t* data, FILE *s)
{
    cur_geometry_index = -1;
    next_atomic_index = 0;
    rw_task = RW_READING_NOTHING;
    rw_framename_index = 0;
    rdepth = -1;

    *data = (dff_t) { 0 };
    dff_read_entry(data, s);
    RW_PRINT("DFF DONE\n");
}

void dff_read_frame_list(dff_t* dst, FILE *s)
{
    rwsection_t sec_data;
    uint32_t n_frames;
    uint32_t end;

    struct {
        vec3f rotation_mat[3];
        vec3f pos;
        uint32_t index;
        uint32_t flags;
    } rwframe;

    fread(&sec_data, 1, sizeof(rwsection_t), s);
    fread(&n_frames, 1, 4, s);

    end = ftell(s) + sec_data.size;

    RW_PRINTF("FRAME_LIST | %d frames\n", n_frames);
    
    dst->n_frames = n_frames;
    dst->frames = sys_malloc(n_frames * sizeof(frame_t));
    
    for (int i = 0; i < n_frames; i++) {
        frame_t* f = &dst->frames[i];
        
        fread(&rwframe, 1, sizeof(rwframe), s);

        f->flags = rwframe.flags;
        f->index = rwframe.index;
        f->pos = rwframe.pos;

        f->rotation_mat = (mat4) { .v = {
            rwframe.rotation_mat[0].x, rwframe.rotation_mat[1].x, rwframe.rotation_mat[2].x, 0,
            rwframe.rotation_mat[0].y, rwframe.rotation_mat[1].y, rwframe.rotation_mat[2].y, 0,
            rwframe.rotation_mat[0].z, rwframe.rotation_mat[1].z, rwframe.rotation_mat[2].z, 0,
            0, 0, 0, 1
        }};
    }
}

typedef struct ebo_triabgle {
    GLint v[3];
} ebo_triangle_t;

enum {
    GEOM_TRI_STRIP = 0x1,
    GEOM_POSITIONS = 0x2,
    GEOM_TEXTURED = 0x4,
    GEOM_PRELIT = 0x8,
    GEOM_NORMALS = 0x10,
    GEOM_LIGHT = 0x20,
    GEOM_MODULATE_MATERIAL_COLO = 0x40,
    GEOM_TEXTURED2 = 0x80,
    GEOM_NATIVE = 0x1000000
};

void dff_read_geometry(dff_t *dst, FILE *s)
{
    typedef struct rwgeometry_header {
        uint32_t format;
        uint32_t num_triangles;
        uint32_t num_vertices;
        uint32_t num_morph_targets;
    } rwgeometry_header_t;

    typedef struct rwtriangle {
        uint16_t vertex2;
        uint16_t vertex1;
        uint16_t material_id;
        uint16_t vertex3;
    } rwtriangle_t;

    rwsection_t sec_data;
    fread(&sec_data, 1, sizeof(rwsection_t), s);

    rwgeometry_header_t header;
    fread(&header, 1, sizeof(header), s);
    
    geometry_t* g = &dst->geometries[++cur_geometry_index];
    RW_PRINTF("GEOMERTY | %d tris | %d verts\n", header.num_triangles, header.num_vertices);

    size_t n_uv_layers = (header.format & 0x00FF0000) >> 16;
    if (n_uv_layers != 0) {
        n_uv_layers = (header.format & GEOM_TEXTURED) ? 1 : ((header.format & GEOM_TEXTURED2) ? 2 : 0);
    }

    glvertex_t* verts = sys_malloc(header.num_vertices * sizeof(glvertex_t));
    for (size_t i = 0; i < header.num_vertices; i++)
        verts[i].color = VEC4F(1.f, 1.f, 1.f, 1.f);

    vec2f** uv_maps = sys_malloc(n_uv_layers * sizeof(vec2f*));

    size_t ebos_allocd = 0;
    vector_t* ebos = NULL;

    if ((header.format & GEOM_NATIVE) == 0) {
        if (header.format & GEOM_PRELIT) {
            for (int i = 0; i < header.num_vertices; i++) {
                uint32_t prelit_color;
                fread(&prelit_color, 1, 4, s);

                verts[i].color.r *= (float)(prelit_color & 0xFF) / 255.f;
                verts[i].color.g *= (float)((prelit_color >> 8) & 0xFF) / 255.f;
                verts[i].color.b *= (float)((prelit_color >> 16) & 0xFF) / 255.f;
                verts[i].color.a *= (float)((prelit_color >> 24) & 0xFF) / 255.f;
            }
        }

        for (size_t i = 0; i < n_uv_layers; i++) {
            uv_maps[i] = sys_malloc(header.num_vertices * sizeof(vec2f));
            fread(uv_maps[i], 1, header.num_vertices * sizeof(vec2f), s);
        }

        for (int i = 0; i < header.num_triangles; i++) {
            rwtriangle_t tri;
            fread(&tri, 1, sizeof(tri), s);

            if (ebos_allocd <= tri.material_id) {
                ebos = sys_realloc(ebos, (tri.material_id + 1) * sizeof(vector_t));

                for (size_t i = ebos_allocd; i <= tri.material_id; i++) {
                    ebos[i] = vector_of(ebo_triangle_t);
                }
                ebos_allocd = tri.material_id + 1;
            }

            ebo_triangle_t* t = vector_emplace_back(&ebos[tri.material_id], ebo_triangle_t);
            t->v[0] = tri.vertex1;
            t->v[1] = tri.vertex2;
            t->v[2] = tri.vertex3;
        }
    }

    for (size_t i = 0; i < header.num_vertices; i++)
        verts[i].uv = uv_maps[0][i];

    for (int i = 0; i < header.num_morph_targets; i++) {
        struct {
            vec3f sphere_pos;
            float sphere_radius;
            uint32_t has_vertices;
            uint32_t has_normals;
        } morph;

        fread(&morph, 1, sizeof(morph), s);

        if (morph.has_vertices) {
            for (int i = 0; i < header.num_vertices; i++) {
                fread(&verts[i].pos, 1, sizeof(vec3f), s);
            }
        }
        if (morph.has_normals) {
            for (int i = 0; i < header.num_vertices; i++) {
                vec3f normal;
                fread(&normal, 1, 4*3, s);
            }
        }
    }

    glwrapGenBuffers(1, &g->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g->vbo);
    glBufferData(GL_ARRAY_BUFFER, header.num_vertices * sizeof(glvertex_t), verts, GL_STATIC_DRAW);
    
    glwrapGenVertexArrays(1, &g->vao);
    glBindVertexArray(g->vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glvertex_t), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glvertex_t), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glvertex_t), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    g->n_draws = ebos_allocd;
    g->draws = sys_malloc(g->n_draws * sizeof(drawcall_t));
    
    for (int i = 0; i < ebos_allocd; i++) {
        if (ebos[i].size) {
            drawcall_t *dc = &g->draws[i];
            
            dc->n_verts = 3 * ebos[i].size;

            glwrapGenBuffers(1, &dc->ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dc->ebo); 
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, dc->n_verts * sizeof(GLuint), ebos[i].data, GL_STATIC_DRAW);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Cleanup
    for (size_t i = 0; i < n_uv_layers; i++) sys_free(uv_maps[i]);
    for (size_t i = 0; i < ebos_allocd; i++) vector_destroy(&ebos[i]);
    
    sys_free(verts);
    sys_free(uv_maps);
    sys_free(ebos);
}

void dff_read_clump(dff_t* dst, FILE *s)
{
    rwsection_t sec_data;
    fread(&sec_data, 1, sizeof(rwsection_t), s);

    struct {
        uint32_t n_atomics;
        uint32_t n_light;
        uint32_t n_cameras;
    } clump;

    fread(&clump, 1, sizeof(clump), s);
    RW_PRINTF("CLUMP | %d atomics\n", clump.n_atomics);

    dst->n_atomics = clump.n_atomics;
    dst->atomics = sys_malloc(clump.n_atomics * sizeof(atomic_t));
}

void dff_read_geometry_list(dff_t *dst, FILE *s)
{
    rwsection_t sec_data;
    uint32_t n_geom;
    
    fread(&sec_data, 1, sizeof(rwsection_t), s);
    fread(&n_geom, 1, 4, s);

    RW_PRINTF("GEOMETRY_LIST | number of geometries = %d\n", n_geom);

    dst->n_geometries = n_geom;
    dst->geometries = sys_malloc(n_geom * sizeof(geometry_t));
}

void dff_read_material_list(dff_t* dst, FILE *s)
{
    rwsection_t sec_data;
    fread(&sec_data, 1, sizeof(rwsection_t), s);

    uint32_t n_materials;
    fread(&n_materials, 1, 4, s);

    for (size_t i = 0; i < n_materials; i++) {
        size_t mat_ind;
        fread(&mat_ind, 1, 4, s);
    }

    RW_PRINTF("MATERIAL_LIST | n_materials = %d\n", n_materials);

    geometry_t *g = &dst->geometries[cur_geometry_index];
    g->n_materials = n_materials;
    g->materials = sys_malloc(n_materials * sizeof(material_t));

    cur_material_index = -1;
}

void dff_read_material(dff_t* dst, FILE *s)
{
    typedef struct rwmaterial {
        int flags;
        unsigned int color;
        int unused;
        int is_textured;
        float ambient;
        float specular;
        float diffuse;
    } rwmaterial_t;

    rwsection_t sec_data;
    rwmaterial_t mat;

    fread(&sec_data, 1, sizeof(rwsection_t), s);
    fread(&mat, 1, sizeof(mat), s);

    RW_PRINTF("MATERIAL | color32 = %X | is_textured = %d\n", mat.color, mat.is_textured);

    dst->geometries[cur_geometry_index].materials[++cur_material_index] = (material_t) {
        .color = mat.color,
        .is_textured = mat.is_textured,
        .ambient = mat.ambient,
        .specular = mat.specular,
        .diffuse = mat.diffuse
    };
}

void dff_read_texture(dff_t *dst, FILE *s)
{
    typedef struct rwtexture {
        uint8_t filtering;
        uint8_t uv;
        uint16_t mipmaps;
    } rwtexture_t;
    
    rwsection_t sec_data;
    rwtexture_t tx;

    fread(&sec_data, 1, sizeof(rwsection_t), s);
    fread(&tx, 1, 4, s);

    rw_task = RW_READING_TX;
    cur_texture_index = -1;

    RW_PRINTF("TEXTURE\n");
}

void dff_read_atomic(dff_t* dst, FILE *s)
{
    typedef struct rwatomic {
        uint32_t frame_index;
        uint32_t geometry_index;
        uint32_t flags;
        uint32_t unused;
    } rwatomic_t;
    
    rwsection_t sec_data;
    rwatomic_t atomic;

    fread(&sec_data, 1, sizeof(sec_data), s);
    fread(&atomic, 1, sizeof(atomic), s);

    RW_PRINTF("ATOMIC | frame %d | geometry %d | flags %X\n", atomic.frame_index, atomic.geometry_index, atomic.flags);

    dst->atomics[next_atomic_index] = (atomic_t) {
        .id = next_atomic_index,
        .frame_index = atomic.frame_index,
        .geometry_index = atomic.geometry_index,
        .flags = atomic.flags
    };

    next_atomic_index++;
}

void dff_read_string(dff_t* dst, FILE *s, int len)
{
    char *str = (char*)sys_malloc(len);
    fread(str, 1, len, s);

    switch (rw_task) {
        case RW_READING_NOTHING:
            RW_PRINTF("STRING '%s'\n", str);
            break;

        case RW_READING_TX:
        {
            textureid_t *tx = &dst->geometries[cur_geometry_index].materials[cur_material_index].tx[++cur_texture_index];
            if (strlen(str) != 0) {
                str8 path;
                str8_create_by_printf(&path, "%s.png", str);

                *tx = gfx_cache_texture(path.data, TEXTURE_LINEAR_FILTER);
                RW_PRINTF("TEXTURE NAME '%s' %d\n", path.data, tx);

                if (!tx) {
                    RENG_LOGF("TEXTURE NOT FOUND: %s\n", path.data);
                }

                str8_destroy(&path);
            }
            else tx = 0;
            break;
        }
        default:
            break;
    }

    sys_free(str);
}

void dff_read_entry(dff_t *dst, FILE *s)
{
    rwsection_t section;

    fread(&section, 1, sizeof(section), s);
    rdepth++;
    
    if (feof(s)) return;

    RW_PRINTF("0x%05X | ", ftell(s));
    for (uint32_t i = 0; i < rdepth; i++) RW_PRINT("    ");
    RW_PRINTF("* ", section.code, section.size);

    int32_t end = ftell(s) + section.size - 12;

    switch (section.code) {
        case 0x2:
            dff_read_string(dst, s, section.size);
            break;
        case 0x0253F2FE:
        {
            char** strptr = &dst->frames[rw_framename_index++].name;
            *strptr = sys_malloc(section.size + 1);
            fread(*strptr, 1, section.size, s);
            (*strptr)[section.size] = 0;
            
            RW_PRINTF("FRAME %s\n", str.data);
        }
            break;
        case 0x3:
            RW_PRINT("EXTENSION\n");
            dff_read_entry(dst, s);            
            break;
        case 0x6:
            dff_read_texture(dst, s);
            break;
        case 0x7:
            dff_read_material(dst, s);
            break;
        case 0x8:
            dff_read_material_list(dst, s);
            break;
        case 0x10:
            dff_read_clump(dst, s);
            break;
        case 0xE:
            dff_read_frame_list(dst, s);
            break;
        case 0xF:
            dff_read_geometry(dst, s);
            break;
        case 0x14:
            dff_read_atomic(dst, s);
            break;
        case 0x1A:
            dff_read_geometry_list(dst, s);
            break;
        default:
            RW_PRINTF("UNKNOWN TYPE %X\n", section.code);
            fseek(s, section.size, SEEK_CUR);
    }

    while (ftell(s) < end)
        dff_read_entry(dst, s);

    rdepth--;
}

void rw_read_texture_dict(const char *name)
{
    WIN32_FIND_DATAA data;
    HANDLE hFind;
    str8 buf;
    
    SetCurrentDirectoryA("models");
    str8_create_by_printf(&buf, "%s\\*", name);
    hFind = FindFirstFileA(buf.data, &data);

    SetCurrentDirectoryA(name);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            size_t len = strlen(data.cFileName);
            if (len <= 4) continue;                 /* ".png" or less */

            if (strcmp(data.cFileName + len - 4, ".png") == 0) {
                gfx_cache_texture(data.cFileName, TEXTURE_LINEAR_FILTER);
            }
        } while (FindNextFileA(hFind, &data));
       
        FindClose(hFind);
    }

    str8_destroy(&buf);
    SetCurrentDirectoryA("..");
    SetCurrentDirectoryA("..");
}

// WTF IS THAT?? WORKS FOR NOW SO DONT TOUCH
void rw_unload_texture_dict(const char *name)
{
    WIN32_FIND_DATAA data;
    HANDLE hFind;
    str8 buf;

    SetCurrentDirectoryA("models");
    str8_create_by_printf(&buf, "%s\\*", name);
    hFind = FindFirstFileA(buf.data, &data);

    SetCurrentDirectoryA(name);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            size_t len = strlen(data.cFileName);
            if (len <= 4) continue;                 /* ".png" or less */

            if (strcmp(data.cFileName + len - 4, ".png") == 0) {
                gfx_uncache_texture(data.cFileName);
            }
        } while (FindNextFileA(hFind, &data));

        FindClose(hFind);
    }

    str8_destroy(&buf);
    SetCurrentDirectoryA("..");
    SetCurrentDirectoryA("..");
}

typedef struct txdasset {
    int used_cnt;
} txdasset_t;

hashtable_t txds;

void rw_cache_texture_dict(const char *name)
{
    hashbucket_t *txd = hashtable_find(&txds, name);

    if (txd) {
        HASHBUCKET_DATA(txd, txdasset_t).used_cnt++;
        return;
    }

    RENG_LOGF("Cached TXD %s", name);
    rw_read_texture_dict(name);
    HASHBUCKET_DATA(hashtable_emplace(&txds, name), txdasset_t).used_cnt = 1;
}

void rw_uncache_texture_dict(const char *name)
{
    hashbucket_t* txd = hashtable_find(&txds, name);
    if (!txd) return;

    HASHBUCKET_DATA(txd, txdasset_t).used_cnt--;
    if (!HASHBUCKET_DATA(txd, txdasset_t).used_cnt) {
        RENG_LOGF("Unached TXD %s", name);
        rw_unload_texture_dict(name);
        hashbucket_empty(txd);
    }
}

void rw_init()
{
    txds = hashtable_of(txdasset_t);
}

void rw_deinit()
{
    for (int i = 0; i < txds.n_buckets * HT_SECTION_LEN; i++) {
        hashbucket_t *b = hashtable_pick_bucket(&txds, i);
        if (b->used) {
            RENG_LOGF("Unloaded TXD: %s (used %d times)", b->name, HASHBUCKET_DATA(b, txdasset_t).used_cnt);
        }
    }

    hashtable_destroy(&txds);
}