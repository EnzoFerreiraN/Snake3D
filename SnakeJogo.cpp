// =============================================================================
//  SnakeJogo.cpp — Snake 3D em C++ com OpenGL / GLUT clássica (glut32)
//  Trabalho Final — Computação Gráfica
// =============================================================================
//
//  COMPILAÇÃO (Windows / glut32 estático — mesmo ambiente do ProjetoFinal.fpj):
//    g++ SnakeJogo.cpp -o ProjetoFinal.exe -std=c++0x -DGLUT_STATIC -static ^
//        -lglut32 -lglu32 -lopengl32 -lwinmm -lgdi32
//
//  TEXTURAS: carregadas de textures/ via stb_image (fallback procedural).
//  MODELOS:  maçã e cabeça da cobra lidos de .obj (fallback cubo).
//
//  CONTROLES:
//    Setas (ou WASD)  — mudar direção da cobra
//    C                — alternar câmera 3ª pessoa ↔ 1ª pessoa ↔ topo
//    R                — reiniciar partida
//    ESC              — sair
//    Arrastar mouse   — orbitar câmera de 3ª pessoa
// =============================================================================

// ─── stb_image — carregamento de JPG/PNG ─────────────────────────────────────
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ─── Includes condicionais (macOS vs Windows/Linux) ──────────────────────────
#ifdef __APPLE__
#  define GL_SILENCE_DEPRECATION
#  include <GLUT/glut.h>
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#else
#  include <GL/glut.h>
#  include <GL/gl.h>
#  include <GL/glu.h>
#endif

// Som via winmm (Windows) ou afplay/POSIX (macOS)
#ifdef _WIN32
#  include <windows.h>
#  include <mmsystem.h>
#  ifdef _MSC_VER
#    pragma comment(lib, "winmm.lib")
#  endif
#elif defined(__APPLE__)
#  include <unistd.h>      // fork, execlp, setpgid, _exit
#  include <signal.h>      // kill, SIGKILL
#  include <sys/wait.h>    // waitpid
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <string>

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

// GL_CLAMP_TO_EDGE pode não estar nos headers antigos do glut32 (OpenGL 1.1)
#ifndef GL_CLAMP_TO_EDGE
#  define GL_CLAMP_TO_EDGE 0x812F
#endif

// =============================================================================
//  CONFIGURAÇÃO — edite aqui para ajustar o jogo
// =============================================================================

#define GRID            30      // células em cada dimensão do grid (20×20)
#define CELL            1.0f   // tamanho de cada célula em unidades GL
#define STEP_MS_INIT    150    // milissegundos por passo (velocidade inicial)
#define STEP_MS_MIN      60    // milissegundos mínimos (velocidade máxima)
#define SPEED_INCR        5    // redução em ms a cada maçã comida
#define APPLES_PER_WALL   5    // maçãs necessárias para surgir a parede central

// ─── Caminhos de textura (fallback procedural se o arquivo não existir) ───────
#define TEX_FLOOR "textures/grass/Grass005_1K-JPG_Color.jpg"
#define TEX_BODY  "textures/snake/Stylized_Scales_003_basecolor.png"
#define TEX_WALL  "textures/wall/PavingStones138_1K-JPG_Color.jpg"
#define TEX_FOOD  "textures/apple/food_apple_01_diff_4k.jpg"
#define TEX_HEAD  "textures/snake/Material_01.png"

// ─── Faces do skybox (ordem: +X -X +Y -Y +Z -Z) ─────────────────────────────
#define SKY_POSX "textures/skybox/posx.jpg"
#define SKY_NEGX "textures/skybox/negx.jpg"
#define SKY_POSY "textures/skybox/posy.jpg"
#define SKY_NEGY "textures/skybox/negy.jpg"
#define SKY_POSZ "textures/skybox/posz.jpg"
#define SKY_NEGZ "textures/skybox/negz.jpg"

// ─── Modelos OBJ ─────────────────────────────────────────────────────────────
#define OBJ_APPLE "textures/apple/food_apple_01_4k.obj"
#define OBJ_HEAD  "textures/snake/snake_head.obj"

// ─── Arquivos de som (silencioso se não existir) ─────────────────────────────
#define SND_EAT      "sounds/eat.wav"
#define SND_PORTAL   "sounds/portal.wav"
#define SND_WALL     "sounds/wall.wav"
#define SND_GAMEOVER "sounds/gameover.wav"
#define SND_BGM      "sounds/bgm.mp3"

// =============================================================================
//  ESTRUTURAS
// =============================================================================

// Célula do grid (posição em coordenadas de grid)
struct Cell {
    int x, z;
    bool operator==(const Cell& o) const { return x == o.x && z == o.z; }
};

// Direção: vetor de deslocamento em células por passo
struct Dir {
    int dx, dz;
    bool operator==(const Dir& o)  const { return dx == o.dx && dz == o.dz; }
    bool isOpposite(const Dir& o)  const { return dx == -o.dx && dz == -o.dz; }
};

// Tipo de célula no grid lógico
enum CellType { CT_EMPTY = 0, CT_WALL = 1 };

// =============================================================================
//  PAREDES E PORTAIS
// =============================================================================

#define WALL_COLUMN (GRID / 2)

static Cell PORTAL_A = { 0, 0 };
static Cell PORTAL_B = { 0, 0 };
static bool wallHorizontal = false;  // false = vertical (coluna), true = horizontal (linha)

// =============================================================================
//  ESTADO GLOBAL DO JOGO
// =============================================================================

static std::vector<Cell> snake;
static Dir  curDir  = {1, 0};
static Dir  nextDir = {1, 0};

static int  gridState[GRID][GRID];

static Cell food;

static bool wallsActive    = false;
static bool portalsActive  = false;
static int  portalCooldown = 0;

static int  score       = 0;
static int  applesEaten = 0;
static bool gameOver    = false;
static int  stepMs      = STEP_MS_INIT;

static int   camMode   = 0;
static float camAngleH = 45.0f;
static float camAngleV = 50.0f;
static float camDist   = 28.0f;
static int   mouseLastX = 0, mouseLastY = 0;
static bool  mouseDown  = false;

static int winW = 800, winH = 600;

// ─── Texturas ─────────────────────────────────────────────────────────────────
static GLuint texFloor = 0, texBody = 0, texWall = 0, texFood = 0, texHead = 0;
static GLuint texSky[6] = {0,0,0,0,0,0};

// ─── Quadric para o corpo da cobra (esferas + cilindros via GLU) ───────────
static GLUquadric* bodyQuad = NULL;

// =============================================================================
//  SOM
// =============================================================================

// PID do processo-filho que toca a BGM em loop no macOS (-1 = inativo)
#ifdef __APPLE__
static pid_t bgmPid = -1;
#endif

static void playSound(const char* filename) {
#ifdef _WIN32
    PlaySoundA(filename, NULL, SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
#elif defined(__APPLE__)
    // Dispara afplay em background (non-blocking); ignora falhas silenciosamente
    char cmd[600];
    snprintf(cmd, sizeof(cmd), "afplay \"%s\" >/dev/null 2>&1 &", filename);
    system(cmd);
#else
    (void)filename;
#endif
}

static bool musicOn = true;

static void startBGM() {
#ifdef _WIN32
    mciSendStringA("close bgm", NULL, 0, NULL);
    mciSendStringA("open \"" SND_BGM "\" type mpegvideo alias bgm", NULL, 0, NULL);
    mciSendStringA("play bgm repeat", NULL, 0, NULL);
#elif defined(__APPLE__)
    // Para BGM anterior (se houver) antes de iniciar nova instância
    if (bgmPid > 0) { kill(-bgmPid, SIGKILL); waitpid(bgmPid, NULL, 0); bgmPid = -1; }
    // Fork: filho entra em loop infinito relançando afplay a cada reprodução
    bgmPid = fork();
    if (bgmPid == 0) {
        setpgid(0, 0); // cria grupo próprio para matar o afplay junto com o loop
        for (;;) {
            pid_t c = fork();
            if (c == 0) { execlp("afplay", "afplay", SND_BGM, (char*)NULL); _exit(1); }
            int st; waitpid(c, &st, 0);
        }
    } else if (bgmPid > 0) {
        setpgid(bgmPid, bgmPid); // pai também define o grupo do filho
    }
#endif
}

static void stopBGM() {
#ifdef _WIN32
    mciSendStringA("close bgm", NULL, 0, NULL);
#elif defined(__APPLE__)
    // Mata todo o grupo de processos (loop-pai + afplay corrente)
    if (bgmPid > 0) { kill(-bgmPid, SIGKILL); waitpid(bgmPid, NULL, 0); bgmPid = -1; }
#endif
}

static void toggleBGM() {
    musicOn = !musicOn;
    if (musicOn) startBGM();
    else         stopBGM();
}

// =============================================================================
//  TEXTURAS — geração procedural (fallback)
// =============================================================================

enum TexKind { TK_CHECKER, TK_BRICK, TK_SCALES, TK_APPLE, TK_HEAD, TK_SKY };

static GLuint makeProceduralTex(TexKind kind) {
    const int S = 64;
    static unsigned char buf[S * S * 4];
    unsigned char* p = buf;

    for (int row = 0; row < S; row++) {
        for (int col = 0; col < S; col++) {
            unsigned char r = 0, g = 0, b = 0, a = 255;
            switch (kind) {
            case TK_CHECKER: {
                bool even = ((row / 8) + (col / 8)) % 2 == 0;
                r = even ? 55  : 90;
                g = even ? 110 : 160;
                b = even ? 55  : 75;
                break;
            }
            case TK_BRICK: {
                int brow = row % 16;
                int bcol = (col + ((row / 16) % 2) * 8) % 16;
                bool grout = brow <= 1 || bcol <= 1;
                r = grout ? 90  : 175;
                g = grout ? 75  : 65;
                b = grout ? 65  : 55;
                break;
            }
            case TK_SCALES: {
                bool sc = ((row / 6) + (col / 6)) % 2 == 0;
                r = sc ? 25  : 70;
                g = sc ? 130 : 190;
                b = sc ? 25  : 55;
                break;
            }
            case TK_APPLE: {
                float cx = col - S * 0.5f;
                float cy = row - S * 0.5f;
                float d  = sqrtf(cx*cx + cy*cy) / (S * 0.5f);
                r = (unsigned char)((1.0f - d * 0.4f) * 215);
                g = (unsigned char)(d * 25.0f);
                b = (unsigned char)(d * 15.0f);
                a = d < 1.0f ? 255 : 0;
                break;
            }
            case TK_HEAD: {
                r = 230; g = 200; b = 40;
                break;
            }
            case TK_SKY: {
                // Gradiente azul céu como fallback
                float t = (float)row / S;
                r = (unsigned char)(30  + t * 80);
                g = (unsigned char)(100 + t * 80);
                b = (unsigned char)(180 + t * 60);
                break;
            }
            }
            *p++ = r; *p++ = g; *p++ = b; *p++ = a;
        }
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, S, S, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return tex;
}

// ─── loadTexture: tenta arquivo real; fallback procedural ────────────────────
// wrap:   GL_REPEAT (objetos) ou GL_CLAMP_TO_EDGE (skybox)
// mipmap: true  → gluBuild2DMipmaps + LINEAR_MIPMAP_LINEAR (padrão)
//         false → glTexImage2D + GL_LINEAR sem mipmap (atlas com fundo preto)
static GLuint loadTexture(const char* path, TexKind fallback,
                          GLint wrap = GL_REPEAT, bool mipmap = true)
{
    int w, h, n;
    unsigned char* data = stbi_load(path, &w, &h, &n, 0);
    if (!data) {
        // arquivo não encontrado ou não legível — usa procedural
        return makeProceduralTex(fallback);
    }

    GLenum fmt = (n == 4) ? GL_RGBA : GL_RGB;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    if (mipmap) {
        gluBuild2DMipmaps(GL_TEXTURE_2D, fmt, w, h, fmt, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    } else {
        // Sem mipmap — evita que níveis reduzidos misturem o fundo preto do atlas
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

    stbi_image_free(data);
    return tex;
}

// =============================================================================
//  OBJ LOADER — malha simples compilada em display list
// =============================================================================

struct Mesh {
    GLuint list;   // 0 = não carregado (usa fallback cubo)
    Mesh() : list(0) {}
};

static Mesh meshApple;
static Mesh meshHead;

// Estrutura temporária para carregar um OBJ
struct ObjData {
    std::vector<float> vx, vy, vz;       // vértices
    std::vector<float> tx, ty;           // UV
    std::vector<float> nx, ny, nz;       // normais

    // Face: cada elemento é um trio (posIdx, uvIdx, normIdx) — base 1
    struct FaceVert { int v, t, n; };
    std::vector<std::vector<FaceVert> > faces;
};

// Lê um token "v/t/n" ou "v//n" ou "v/t" ou "v"
static ObjData::FaceVert parseFaceVert(const char* tok) {
    ObjData::FaceVert fv = {0,0,0};
    // sscanf aceita campos vazios tratando-os como 0 somente se usarmos "%d/%d/%d"
    // mas "%d//%d" falha. Fazemos parse manual:
    int v=0, t=0, n=0;
    const char* p = tok;
    // v
    while (*p && *p != '/') { v = v*10 + (*p-'0'); p++; }
    if (*p == '/') {
        p++;
        // t (pode estar vazio antes do segundo '/')
        while (*p && *p != '/') { t = t*10 + (*p-'0'); p++; }
        if (*p == '/') {
            p++;
            // n
            while (*p && *p != '/' && *p != ' ' && *p != '\0') { n = n*10 + (*p-'0'); p++; }
        }
    }
    fv.v = v; fv.t = t; fv.n = n;
    return fv;
}

// Carrega um .obj e o compila como display list normalizada para caber em escala
// targetSize: o maior lado da bounding box ficará com este tamanho.
static bool loadOBJ(const char* path, Mesh& mesh, float targetSize) {
    FILE* f = fopen(path, "r");
    if (!f) return false;

    ObjData obj;
    char line[512];

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "vt ", 3) == 0) {
            float u=0, v=0;
            sscanf(line+3, "%f %f", &u, &v);
            obj.tx.push_back(u);
            obj.ty.push_back(v);
        } else if (strncmp(line, "vn ", 3) == 0) {
            float nx=0,ny=0,nz=0;
            sscanf(line+3, "%f %f %f", &nx, &ny, &nz);
            obj.nx.push_back(nx);
            obj.ny.push_back(ny);
            obj.nz.push_back(nz);
        } else if (strncmp(line, "v ", 2) == 0) {
            float x=0,y=0,z=0;
            sscanf(line+2, "%f %f %f", &x, &y, &z);
            obj.vx.push_back(x);
            obj.vy.push_back(y);
            obj.vz.push_back(z);
        } else if (strncmp(line, "f ", 2) == 0) {
            // Tokeniza os vértices da face
            std::vector<ObjData::FaceVert> poly;
            char* ctx = NULL;
            char tmp[512];
            strncpy(tmp, line+2, sizeof(tmp)-1);
            tmp[sizeof(tmp)-1] = '\0';
            // strtok_r não é padrão C++03; usar strtok com global state
            char* tok = strtok(tmp, " \t\r\n");
            while (tok) {
                poly.push_back(parseFaceVert(tok));
                tok = strtok(NULL, " \t\r\n");
            }
            if (poly.size() >= 3)
                obj.faces.push_back(poly);
        }
    }
    fclose(f);

    if (obj.vx.empty() || obj.faces.empty()) return false;

    // ── Calcula bounding box e centro ────────────────────────────────────────
    float minX = obj.vx[0], maxX = obj.vx[0];
    float minY = obj.vy[0], maxY = obj.vy[0];
    float minZ = obj.vz[0], maxZ = obj.vz[0];
    for (int i = 1; i < (int)obj.vx.size(); i++) {
        if (obj.vx[i] < minX) minX = obj.vx[i];
        if (obj.vx[i] > maxX) maxX = obj.vx[i];
        if (obj.vy[i] < minY) minY = obj.vy[i];
        if (obj.vy[i] > maxY) maxY = obj.vy[i];
        if (obj.vz[i] < minZ) minZ = obj.vz[i];
        if (obj.vz[i] > maxZ) maxZ = obj.vz[i];
    }
    float cx = (minX + maxX) * 0.5f;
    float cy = (minY + maxY) * 0.5f;
    float cz = (minZ + maxZ) * 0.5f;
    float sizeX = maxX - minX;
    float sizeY = maxY - minY;
    float sizeZ = maxZ - minZ;
    float biggest = sizeX;
    if (sizeY > biggest) biggest = sizeY;
    if (sizeZ > biggest) biggest = sizeZ;
    float scale = (biggest > 0.0f) ? (targetSize / biggest) : 1.0f;

    // ── Compila display list ──────────────────────────────────────────────────
    GLuint dl = glGenLists(1);
    glNewList(dl, GL_COMPILE);
    glBegin(GL_TRIANGLES);
    for (int fi = 0; fi < (int)obj.faces.size(); fi++) {
        const std::vector<ObjData::FaceVert>& poly = obj.faces[fi];
        // Fan triangulation: (0,1,2), (0,2,3), (0,3,4), ...
        for (int k = 1; k+1 < (int)poly.size(); k++) {
            const ObjData::FaceVert* fvs[3] = { &poly[0], &poly[k], &poly[k+1] };
            for (int j = 0; j < 3; j++) {
                const ObjData::FaceVert& fv = *fvs[j];
                // Normal (base-1, pode ser 0 se ausente)
                if (fv.n > 0 && fv.n <= (int)obj.nx.size()) {
                    glNormal3f(obj.nx[fv.n-1], obj.ny[fv.n-1], obj.nz[fv.n-1]);
                }
                // UV
                if (fv.t > 0 && fv.t <= (int)obj.tx.size()) {
                    glTexCoord2f(obj.tx[fv.t-1], obj.ty[fv.t-1]);
                }
                // Vértice (centrado e escalado)
                int vi = fv.v - 1;
                glVertex3f((obj.vx[vi] - cx) * scale,
                           (obj.vy[vi] - cy) * scale,
                           (obj.vz[vi] - cz) * scale);
            }
        }
    }
    glEnd();
    glEndList();

    mesh.list = dl;
    return true;
}

// Carrega as duas malhas OBJ
static void loadMeshes() {
    // Maçã: bounding box real ~0.10 unidades; queremos ~0.7 * CELL
    loadOBJ(OBJ_APPLE, meshApple, CELL * 0.70f);
    // Cabeça: bounding box real ~2.7 unidades; queremos ~1.8 * CELL
    // (normalizado pelo eixo maior → largura/altura casamcom o tubo do corpo)
    loadOBJ(OBJ_HEAD,  meshHead,  CELL * 2.1f);
}

// =============================================================================
//  LÓGICA DO JOGO — helpers
// =============================================================================

static bool cellInGrid(int x, int z) {
    return x >= 0 && x < GRID && z >= 0 && z < GRID;
}

static bool cellInSnake(Cell c) {
    for (int i = 0; i < (int)snake.size(); i++)
        if (snake[i] == c) return true;
    return false;
}

static void spawnFood() {
    Cell c;
    int tries = 0;
    do {
        c.x = rand() % GRID;
        c.z = rand() % GRID;
        if (++tries > GRID * GRID * 4) break;
    } while (cellInSnake(c)
          || gridState[c.x][c.z] == CT_WALL
          || (portalsActive && (c == PORTAL_A || c == PORTAL_B)));
    food = c;
}

static void spawnPortals() {
    const int maxTries = GRID * GRID * 4;
    // Interior jogavel: indices de 1 a GRID-2 (borda e' parede)
    const int lo = 1;
    const int hi = GRID - 2;       // inclusive
    Cell c;
    int tries;

    if (!wallHorizontal) {
        // Parede vertical: divide em metade esquerda (x < WALL_COLUMN)
        //                  e metade direita (x > WALL_COLUMN)
        tries = 0;
        do {
            c.x = lo + rand() % (WALL_COLUMN - lo);   // [1, WALL_COLUMN)
            c.z = lo + rand() % (hi - lo + 1);        // [1, GRID-2]
            if (++tries > maxTries) { c.x = lo; c.z = GRID / 2; break; }
        } while (cellInSnake(c) || gridState[c.x][c.z] == CT_WALL || c == food);
        PORTAL_A = c;

        tries = 0;
        do {
            c.x = WALL_COLUMN + 1 + rand() % (hi - WALL_COLUMN);  // (WALL_COLUMN, GRID-2]
            c.z = lo + rand() % (hi - lo + 1);
            if (++tries > maxTries) { c.x = hi; c.z = GRID / 2; break; }
        } while (cellInSnake(c) || gridState[c.x][c.z] == CT_WALL || c == food || c == PORTAL_A);
        PORTAL_B = c;
    } else {
        // Parede horizontal: divide em metade superior (z < WALL_COLUMN)
        //                    e metade inferior (z > WALL_COLUMN)
        tries = 0;
        do {
            c.x = lo + rand() % (hi - lo + 1);        // [1, GRID-2]
            c.z = lo + rand() % (WALL_COLUMN - lo);   // [1, WALL_COLUMN)
            if (++tries > maxTries) { c.x = GRID / 2; c.z = lo; break; }
        } while (cellInSnake(c) || gridState[c.x][c.z] == CT_WALL || c == food);
        PORTAL_A = c;

        tries = 0;
        do {
            c.x = lo + rand() % (hi - lo + 1);
            c.z = WALL_COLUMN + 1 + rand() % (hi - WALL_COLUMN);  // (WALL_COLUMN, GRID-2]
            if (++tries > maxTries) { c.x = GRID / 2; c.z = hi; break; }
        } while (cellInSnake(c) || gridState[c.x][c.z] == CT_WALL || c == food || c == PORTAL_A);
        PORTAL_B = c;
    }
}

static void spawnWall() {
    if (wallsActive) return;

    // Sorteia orientacao 50/50: vertical (coluna) ou horizontal (linha)
    wallHorizontal = (rand() % 2 == 0);
    if (wallHorizontal) {
        // Parede horizontal: preenche a linha central (z == WALL_COLUMN)
        for (int x = 0; x < GRID; x++)
            gridState[x][WALL_COLUMN] = CT_WALL;
    } else {
        // Parede vertical: preenche a coluna central (x == WALL_COLUMN)
        for (int z = 0; z < GRID; z++)
            gridState[WALL_COLUMN][z] = CT_WALL;
    }

    wallsActive = true;
    spawnPortals();
    portalsActive = true;
    playSound(SND_WALL);
}

static void initGame() {
    memset(gridState, 0, sizeof(gridState));
    wallsActive    = false;
    portalsActive  = false;
    portalCooldown = 0;
    wallHorizontal = false;

    // Borda solida do mapa: anel de CT_WALL em torno de todo o grid
    for (int i = 0; i < GRID; i++) {
        gridState[0][i]      = CT_WALL;  // coluna esquerda
        gridState[GRID-1][i] = CT_WALL;  // coluna direita
        gridState[i][0]      = CT_WALL;  // linha superior
        gridState[i][GRID-1] = CT_WALL;  // linha inferior
    }

    snake.clear();
    Cell h  = { GRID/2,     GRID/2 };
    Cell b1 = { GRID/2 - 1, GRID/2 };
    Cell b2 = { GRID/2 - 2, GRID/2 };
    snake.push_back(h);
    snake.push_back(b1);
    snake.push_back(b2);

    curDir  = Dir(); curDir.dx  = 1; curDir.dz  = 0;
    nextDir = Dir(); nextDir.dx = 1; nextDir.dz = 0;

    score       = 0;
    applesEaten = 0;
    gameOver    = false;
    stepMs      = STEP_MS_INIT;

    spawnFood();
}

// =============================================================================
//  PORTAL
// =============================================================================

static bool checkPortal(Cell& head) {
    if (!portalsActive || portalCooldown > 0) return false;

    bool hitA = (head == PORTAL_A);
    bool hitB = (head == PORTAL_B);
    if (!hitA && !hitB) return false;

    Cell exitPortal = hitA ? PORTAL_B : PORTAL_A;
    Cell afterExit  = { exitPortal.x + curDir.dx, exitPortal.z + curDir.dz };

    if (!cellInGrid(afterExit.x, afterExit.z)
        || gridState[afterExit.x][afterExit.z] == CT_WALL)
        afterExit = exitPortal;

    head = afterExit;
    portalCooldown = 3;
    playSound(SND_PORTAL);
    return true;
}

// =============================================================================
//  PASSO DO JOGO
// =============================================================================

static void step() {
    if (gameOver) return;

    curDir = nextDir;

    Cell head = snake[0];
    head.x += curDir.dx;
    head.z += curDir.dz;

    checkPortal(head);

    if (portalCooldown > 0) portalCooldown--;

    if (!cellInGrid(head.x, head.z)) {
        gameOver = true;
        playSound(SND_GAMEOVER);
        return;
    }

    if (gridState[head.x][head.z] == CT_WALL) {
        gameOver = true;
        playSound(SND_GAMEOVER);
        return;
    }

    for (int i = 0; i < (int)snake.size() - 1; i++) {
        if (snake[i] == head) {
            gameOver = true;
            playSound(SND_GAMEOVER);
            return;
        }
    }

    snake.insert(snake.begin(), head);

    if (head == food) {
        score += 10;
        applesEaten++;
        playSound(SND_EAT);

        if (stepMs > STEP_MS_MIN)
            stepMs -= SPEED_INCR;

        if (applesEaten % APPLES_PER_WALL == 0)
            spawnWall();

        spawnFood();
    } else {
        snake.pop_back();
    }
}

static void timer(int) {
    step();
    if (!gameOver)
        glutTimerFunc((unsigned int)stepMs, timer, 0);
    glutPostRedisplay();
}

// =============================================================================
//  RENDERIZAÇÃO — utilitários de coordenadas e geometria
// =============================================================================

static inline float gx(int x) { return (x - GRID * 0.5f + 0.5f) * CELL; }
static inline float gz(int z) { return (z - GRID * 0.5f + 0.5f) * CELL; }

// Cubo texturizado (usado como fallback e para o corpo da cobra)
static void drawCubeTex(float sx, float sy, float sz) {
    float hx = sx * 0.5f, hy = sy * 0.5f, hz = sz * 0.5f;
    glBegin(GL_QUADS);
    glNormal3f(0,0,1);
    glTexCoord2f(0,0); glVertex3f(-hx,-hy, hz);
    glTexCoord2f(1,0); glVertex3f( hx,-hy, hz);
    glTexCoord2f(1,1); glVertex3f( hx, hy, hz);
    glTexCoord2f(0,1); glVertex3f(-hx, hy, hz);

    glNormal3f(0,0,-1);
    glTexCoord2f(0,0); glVertex3f( hx,-hy,-hz);
    glTexCoord2f(1,0); glVertex3f(-hx,-hy,-hz);
    glTexCoord2f(1,1); glVertex3f(-hx, hy,-hz);
    glTexCoord2f(0,1); glVertex3f( hx, hy,-hz);

    glNormal3f(-1,0,0);
    glTexCoord2f(0,0); glVertex3f(-hx,-hy,-hz);
    glTexCoord2f(1,0); glVertex3f(-hx,-hy, hz);
    glTexCoord2f(1,1); glVertex3f(-hx, hy, hz);
    glTexCoord2f(0,1); glVertex3f(-hx, hy,-hz);

    glNormal3f(1,0,0);
    glTexCoord2f(0,0); glVertex3f( hx,-hy, hz);
    glTexCoord2f(1,0); glVertex3f( hx,-hy,-hz);
    glTexCoord2f(1,1); glVertex3f( hx, hy,-hz);
    glTexCoord2f(0,1); glVertex3f( hx, hy, hz);

    glNormal3f(0,1,0);
    glTexCoord2f(0,0); glVertex3f(-hx, hy, hz);
    glTexCoord2f(1,0); glVertex3f( hx, hy, hz);
    glTexCoord2f(1,1); glVertex3f( hx, hy,-hz);
    glTexCoord2f(0,1); glVertex3f(-hx, hy,-hz);

    glNormal3f(0,-1,0);
    glTexCoord2f(0,0); glVertex3f(-hx,-hy,-hz);
    glTexCoord2f(1,0); glVertex3f( hx,-hy,-hz);
    glTexCoord2f(1,1); glVertex3f( hx,-hy, hz);
    glTexCoord2f(0,1); glVertex3f(-hx,-hy, hz);
    glEnd();
}

// =============================================================================
//  RENDERIZAÇÃO — skybox
// =============================================================================
//
//  Faces:  0=+X  1=-X  2=+Y  3=-Y  4=+Z  5=-Z
//  O cubo tem lado 2*R, centrado no olho. Iluminação desligada para que
//  as cores das texturas apareçam puras (céu não é iluminado pela luz da cena).
//  glDepthMask(FALSE) faz o skybox ser pintado mas não escreve no Z-buffer,
//  portanto todos os objetos da cena ficam "na frente" dele.

static void drawSkybox() {
    // Extrai posição atual do olho da matriz de modelview
    GLfloat mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    // A posição do olho é -R^T * t  (coluna 3 da inversa)
    float eyeX = -(mv[0]*mv[12] + mv[1]*mv[13] + mv[2]*mv[14]);
    float eyeY = -(mv[4]*mv[12] + mv[5]*mv[13] + mv[6]*mv[14]);
    float eyeZ = -(mv[8]*mv[12] + mv[9]*mv[13] + mv[10]*mv[14]);

    const float R = 200.0f; // tamanho do skybox

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);   // skybox sempre atrás de tudo
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);

    glPushMatrix();
    glTranslatef(eyeX, eyeY, eyeZ); // centraliza no olho

    // Cada face: texSky[i], wrap GL_CLAMP_TO_EDGE (sem costuras)
    // +X
    glBindTexture(GL_TEXTURE_2D, texSky[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBegin(GL_QUADS);
    glTexCoord2f(0,1); glVertex3f( R,-R,-R);
    glTexCoord2f(0,0); glVertex3f( R, R,-R);
    glTexCoord2f(1,0); glVertex3f( R, R, R);
    glTexCoord2f(1,1); glVertex3f( R,-R, R);
    glEnd();

    // -X
    glBindTexture(GL_TEXTURE_2D, texSky[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBegin(GL_QUADS);
    glTexCoord2f(0,1); glVertex3f(-R,-R, R);
    glTexCoord2f(0,0); glVertex3f(-R, R, R);
    glTexCoord2f(1,0); glVertex3f(-R, R,-R);
    glTexCoord2f(1,1); glVertex3f(-R,-R,-R);
    glEnd();

    // +Y (topo)
    glBindTexture(GL_TEXTURE_2D, texSky[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex3f(-R, R,-R);
    glTexCoord2f(0,1); glVertex3f(-R, R, R);
    glTexCoord2f(1,1); glVertex3f( R, R, R);
    glTexCoord2f(1,0); glVertex3f( R, R,-R);
    glEnd();

    // -Y (fundo)
    glBindTexture(GL_TEXTURE_2D, texSky[3]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex3f(-R,-R, R);
    glTexCoord2f(0,1); glVertex3f(-R,-R,-R);
    glTexCoord2f(1,1); glVertex3f( R,-R,-R);
    glTexCoord2f(1,0); glVertex3f( R,-R, R);
    glEnd();

    // +Z
    glBindTexture(GL_TEXTURE_2D, texSky[4]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBegin(GL_QUADS);
    glTexCoord2f(0,1); glVertex3f( R,-R, R);
    glTexCoord2f(0,0); glVertex3f( R, R, R);
    glTexCoord2f(1,0); glVertex3f(-R, R, R);
    glTexCoord2f(1,1); glVertex3f(-R,-R, R);
    glEnd();

    // -Z
    glBindTexture(GL_TEXTURE_2D, texSky[5]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBegin(GL_QUADS);
    glTexCoord2f(0,1); glVertex3f(-R,-R,-R);
    glTexCoord2f(0,0); glVertex3f(-R, R,-R);
    glTexCoord2f(1,0); glVertex3f( R, R,-R);
    glTexCoord2f(1,1); glVertex3f( R,-R,-R);
    glEnd();

    glPopMatrix();
    glPopAttrib();
}

// =============================================================================
//  RENDERIZAÇÃO — chão com textura de grama
// =============================================================================

static void drawFloor() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texFloor);
    glColor3f(1.0f, 1.0f, 1.0f);

    float half   = GRID * CELL * 0.5f;
    float yFloor = -CELL * 0.5f;
    // Repetição de 4x para a grama ter escala visível (era GRID=20, agora 4)
    float rep    = 4.0f;

    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0,   0  ); glVertex3f(-half, yFloor, -half);
    glTexCoord2f(rep, 0  ); glVertex3f( half, yFloor, -half);
    glTexCoord2f(rep, rep); glVertex3f( half, yFloor,  half);
    glTexCoord2f(0,   rep); glVertex3f(-half, yFloor,  half);
    glEnd();

    // Borda visual do grid
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glColor3f(0.4f, 0.6f, 0.4f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-half, yFloor + 0.01f, -half);
    glVertex3f( half, yFloor + 0.01f, -half);
    glVertex3f( half, yFloor + 0.01f,  half);
    glVertex3f(-half, yFloor + 0.01f,  half);
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

// =============================================================================
//  RENDERIZAÇÃO — cobra
//
//  Cabeça: modelo OBJ texturizado orientado para curDir (ou cubo-fallback).
//  Corpo:  tubo liso — esferas nas juntas + cilindros de ligação, com afinamento
//          gradual do pescoço até a cauda. Textura de escamas (texBody) mapeada
//          automaticamente pelo GLU quadric.
// =============================================================================

// Raio do segmento i de um total n: interpola pescoço → cauda
static inline float bodyRadiusAt(int i, int n) {
    const float rNeck = CELL * 0.42f;   // grosso no pescoço
    const float rTail = CELL * 0.16f;   // fino na cauda
    if (n <= 1) return rNeck;
    float t = (float)(i - 1) / (float)(n - 1); // 0 no pescoço, 1 na cauda
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return rNeck + (rTail - rNeck) * t;
}

// Posição-mundo (y = 0) do segmento de índice k
static inline void snakeSegPos(int k, float& wx, float& wz) {
    wx = gx(snake[k].x);
    wz = gz(snake[k].z);
}

static void drawSnake() {
    if (snake.empty()) return;

    glEnable(GL_TEXTURE_2D);

    // ── Cabeça (i = 0) — modelo OBJ texturizado, orientado para curDir ────────
    {
        glBindTexture(GL_TEXTURE_2D, texHead);
        glColor3f(1.0f, 1.0f, 1.0f);

        glPushMatrix();
        glTranslatef(gx(snake[0].x), 0.0f, gz(snake[0].z));

        // snake_head.obj: eixo principal em +X. Rotação: atan2(-dz, dx) → Y
        float yawDeg = (float)(atan2((double)-curDir.dz,
                                     (double) curDir.dx) * 180.0 / M_PI);
        glRotatef(yawDeg, 0.0f, 1.0f, 0.0f);

        if (meshHead.list) {
            glCallList(meshHead.list);
        } else {
            glColor3f(1.0f, 0.92f, 0.35f);
            drawCubeTex(CELL * 0.92f, CELL * 0.92f, CELL * 0.92f);
        }
        glPopMatrix();
    }

    // ── Corpo — tubo liso com afinamento ─────────────────────────────────────
    if (snake.size() < 2) { glDisable(GL_TEXTURE_2D); return; }

    glBindTexture(GL_TEXTURE_2D, texBody);
    glColor3f(1.0f, 1.0f, 1.0f);

    int n = (int)snake.size(); // total de segmentos (inclui cabeça no índice 0)

    // --- Esferas nas juntas (segmentos 1 … n-1) ---
    // Cobrem as curvas; raio baseado em bodyRadiusAt (i=1 → pescoço, i=n-1 → cauda)
    for (int i = 1; i < n; i++) {
        float wx, wz;
        snakeSegPos(i, wx, wz);
        float r = bodyRadiusAt(i, n);

        glPushMatrix();
        glTranslatef(wx, 0.0f, wz);
        gluSphere(bodyQuad, r, 14, 12);
        glPopMatrix();
    }

    // --- Cilindros de ligação entre segmentos consecutivos ---
    // Ligação k=(i-1) → i, para i = 1 … n-1.
    // Inclui a ligação pescoço→cabeça (i=1 une snake[0] a snake[1]).
    for (int i = 1; i < n; i++) {
        float ax, az, bx, bz;
        snakeSegPos(i-1, ax, az);
        snakeSegPos(i,   bx, bz);

        float dx = bx - ax;
        float dz = bz - az;
        // Comprimento esperado = CELL; os segmentos são sempre adjacentes.

        // gluCylinder aponta no +Z local; temos de rotacionar para (dx, 0, dz).
        // Como o vetor é puramente horizontal, basta girar em torno de Y.
        float yawDeg = (float)(atan2((double)dx, (double)dz) * 180.0 / M_PI);

        // Raio no início (segmento i-1) e no fim (segmento i) do cilindro
        // Para a junção cabeça→pescoço, i-1==0 logo r0 = rNeck é o mais gordo
        float r0 = (i == 1) ? CELL * 0.42f : bodyRadiusAt(i-1, n);
        float r1 = bodyRadiusAt(i, n);

        glPushMatrix();
        glTranslatef(ax, 0.0f, az);
        glRotatef(yawDeg, 0.0f, 1.0f, 0.0f);
        gluCylinder(bodyQuad, r0, r1, CELL, 14, 1);
        glPopMatrix();
    }

    glDisable(GL_TEXTURE_2D);
}

// =============================================================================
//  RENDERIZAÇÃO — comida (maçã 3D flutuante e rotativa)
// =============================================================================

static void drawFood() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texFood);
    glColor3f(1.0f, 1.0f, 1.0f);

    int t     = glutGet(GLUT_ELAPSED_TIME);
    float angle = t * 0.06f;
    float hover = sinf(t * 0.003f) * 0.12f;

    glPushMatrix();
    glTranslatef(gx(food.x), hover, gz(food.z));
    glRotatef(angle, 0, 1, 0);

    if (meshApple.list) {
        glCallList(meshApple.list);
    } else {
        // fallback cubo vermelho
        glColor3f(1.0f, 0.15f, 0.15f);
        drawCubeTex(CELL * 0.65f, CELL * 0.65f, CELL * 0.65f);
    }

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

// =============================================================================
//  RENDERIZAÇÃO — parede central (cubos com textura de pedra/piso)
// =============================================================================

static void drawWalls() {
    // Desenha todas as celulas CT_WALL do gridState (borda permanente + parede central)
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texWall);
    glColor3f(1.0f, 1.0f, 1.0f);

    for (int x = 0; x < GRID; x++) {
        for (int z = 0; z < GRID; z++) {
            if (gridState[x][z] != CT_WALL) continue;
            glPushMatrix();
            glTranslatef(gx(x), CELL * 0.5f, gz(z));
            drawCubeTex(CELL * 0.95f, CELL * 2.0f, CELL * 0.95f);
            glPopMatrix();
        }
    }

    glDisable(GL_TEXTURE_2D);
}

// =============================================================================
//  RENDERIZAÇÃO — portais (esferas animadas com efeito de brilho "glow")
// =============================================================================

static void drawPortal(float wx, float wz,
                       float coreR, float coreG, float coreB,
                       float ringR, float ringG, float ringB)
{
    float t = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    glPushMatrix();
    glTranslatef(wx, 0.7f, wz);

    // Núcleo sólido pulsante
    float pulse = 0.30f + 0.05f * sinf(t * 3.0f);
    glPushMatrix();
        glRotatef(t * 60.0f, 0.0f, 1.0f, 0.0f);
        glColor4f(coreR, coreG, coreB, 0.9f);
        glutSolidSphere(pulse, 24, 24);
    glPopMatrix();

    // Esfera externa wireframe
    glPushMatrix();
        glRotatef(-t * 40.0f, 0.3f, 1.0f, 0.2f);
        glColor4f(coreR, coreG, coreB, 0.35f);
        glutWireSphere(0.55f, 12, 12);
    glPopMatrix();

    // Anéis cruzados
    glColor4f(ringR, ringG, ringB, 0.6f);
    glPushMatrix();
        glRotatef(t * 90.0f, 0.0f, 1.0f, 0.0f);
        glRotatef(90.0f,     1.0f, 0.0f, 0.0f);
        glutSolidTorus(0.04f, 0.50f, 8, 24);
    glPopMatrix();
    glPushMatrix();
        glRotatef(-t * 70.0f, 0.0f, 1.0f, 0.0f);
        glutSolidTorus(0.04f, 0.50f, 8, 24);
    glPopMatrix();

    // Partículas orbitando
    glPointSize(4.0f);
    glBegin(GL_POINTS);
    const int NP = 24;
    for (int i = 0; i < NP; i++) {
        float a   = (float)i / NP * 2.0f * (float)M_PI + t * 1.5f;
        float rad = 0.60f + 0.08f * sinf(t * 2.0f + (float)i);
        float px  = cosf(a) * rad;
        float pz  = sinf(a) * rad;
        float py  = 0.18f * sinf(t * 2.5f + (float)i * 0.7f);
        if (i % 2 == 0) glColor4f(1.0f, 0.5f, 0.0f, 0.9f);
        else            glColor4f(0.2f, 0.5f, 1.0f, 0.9f);
        glVertex3f(px, py, pz);
    }
    glEnd();

    glPopMatrix();
    glDepthMask(GL_TRUE);
    glPopAttrib();
}

static void drawPortals() {
    if (!portalsActive) return;

    drawPortal(gx(PORTAL_A.x), gz(PORTAL_A.z),
               1.0f, 0.5f, 0.0f,
               0.2f, 0.5f, 1.0f);

    drawPortal(gx(PORTAL_B.x), gz(PORTAL_B.z),
               0.2f, 0.5f, 1.0f,
               1.0f, 0.5f, 0.0f);
}

// =============================================================================
//  CÂMERAS
// =============================================================================

static void setupCameraPerspective() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)winW / (double)winH, 0.1, 500.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float radH = (float)(camAngleH * M_PI / 180.0);
    float radV = (float)(camAngleV * M_PI / 180.0);
    float eyeX = camDist * cosf(radV) * sinf(radH);
    float eyeY = camDist * sinf(radV);
    float eyeZ = camDist * cosf(radV) * cosf(radH);

    gluLookAt(eyeX, eyeY, eyeZ,
              0.0,  0.0,  0.0,
              0.0,  1.0,  0.0);
}

static void setupCameraFirstPerson() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(75.0, (double)winW / (double)winH, 0.05, 500.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (snake.empty()) return;

    float eyeX = gx(snake[0].x);
    float eyeY = CELL * 0.35f;
    float eyeZ = gz(snake[0].z);

    float atX = eyeX + (float)curDir.dx * CELL;
    float atZ = eyeZ + (float)curDir.dz * CELL;

    gluLookAt(eyeX, eyeY, eyeZ,
              atX,  eyeY, atZ,
              0.0,  1.0,  0.0);
}

static void setupCameraTopDown() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)winW / (double)winH, 0.1, 500.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float eyeY = GRID * CELL * 1.3f;
    gluLookAt(0.0f, eyeY, 0.0f,
              0.0f, 0.0f, 0.0f,
              0.0f, 0.0f, -1.0f);
}

// =============================================================================
//  HUD
// =============================================================================

static void drawString2D(float x, float y, const char* s, void* font) {
    glRasterPos2f(x, y);
    while (*s) glutBitmapCharacter(font, (unsigned char)*s++);
}

static void drawHUD() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    char buf[160];
    glColor3f(1.0f, 1.0f, 1.0f);
    const char* camName = (camMode == 0) ? "3a pessoa"
                        : (camMode == 1) ? "1a pessoa"
                        : "topo";
    sprintf(buf, "Score: %d   Macas: %d   Camera: %s",
            score, applesEaten, camName);
    drawString2D(10.0f, (float)(winH - 22), buf, GLUT_BITMAP_HELVETICA_18);

    if (!wallsActive && applesEaten > 0) {
        int falta = APPLES_PER_WALL - (applesEaten % APPLES_PER_WALL);
        glColor3f(1.0f, 0.9f, 0.3f);
        sprintf(buf, "Parede em %d maca(s)!", falta);
        drawString2D(10.0f, (float)(winH - 44), buf, GLUT_BITMAP_HELVETICA_12);
    }

    glColor3f(0.75f, 0.75f, 0.75f);
    drawString2D(10.0f, 8.0f,
        "Setas/WASD: mover  |  C: camera  |  M: musica  |  R: reiniciar  |  ESC: sair",
        GLUT_BITMAP_HELVETICA_12);

    if (gameOver) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f, 0.0f, 0.0f, 0.65f);
        glBegin(GL_QUADS);
        glVertex2f(winW * 0.22f, winH * 0.33f);
        glVertex2f(winW * 0.78f, winH * 0.33f);
        glVertex2f(winW * 0.78f, winH * 0.67f);
        glVertex2f(winW * 0.22f, winH * 0.67f);
        glEnd();
        glDisable(GL_BLEND);

        glColor3f(1.0f, 0.18f, 0.18f);
        drawString2D(winW * 0.36f, winH * 0.56f,
                     "GAME OVER!", GLUT_BITMAP_TIMES_ROMAN_24);

        sprintf(buf, "Pontuacao final: %d  (Macas: %d)", score, applesEaten);
        glColor3f(1.0f, 1.0f, 0.8f);
        drawString2D(winW * 0.30f, winH * 0.48f, buf, GLUT_BITMAP_HELVETICA_18);

        glColor3f(0.8f, 0.8f, 0.8f);
        drawString2D(winW * 0.35f, winH * 0.40f,
                     "Pressione R para reiniciar",
                     GLUT_BITMAP_HELVETICA_12);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

// =============================================================================
//  DISPLAY — função principal de renderização
// =============================================================================

static void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Configura câmera
    if      (camMode == 0) setupCameraPerspective();
    else if (camMode == 1) setupCameraFirstPerson();
    else                   setupCameraTopDown();

    // ── Skybox (desenhado antes de tudo, sem escrita no Z) ───────────────────
    drawSkybox();

    // Reativa depth test para a cena 3D
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    // Posiciona luz principal
    GLfloat lightPos[] = { (float)GRID * 0.25f, (float)GRID * 1.8f,
                           (float)GRID * 0.25f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // Cena 3D
    drawFloor();
    drawWalls();
    drawPortals();
    drawFood();
    drawSnake();

    // HUD 2D
    drawHUD();

    glutSwapBuffers();
}

// =============================================================================
//  CARREGAMENTO DE TEXTURAS E MODELOS
// =============================================================================

static void loadTextures() {
    // Texturas de cor — fallback procedural se arquivo ausente
    texFloor = loadTexture(TEX_FLOOR, TK_CHECKER);
    texBody  = loadTexture(TEX_BODY,  TK_SCALES);
    texWall  = loadTexture(TEX_WALL,  TK_BRICK);
    // Atlas da maçã tem fundo preto — desliga mipmap para evitar manchas escuras
    texFood  = loadTexture(TEX_FOOD,  TK_APPLE, GL_REPEAT, /*mipmap=*/false);
    // Cabeça usa a mesma textura de escamas do corpo (Material_01 é thumbnail)
    texHead  = texBody;

    // Skybox — 6 faces (fallback: gradiente azul)
    const char* skyPaths[6] = {
        SKY_POSX, SKY_NEGX,
        SKY_POSY, SKY_NEGY,
        SKY_POSZ, SKY_NEGZ
    };
    for (int i = 0; i < 6; i++)
        texSky[i] = loadTexture(skyPaths[i], TK_SKY, GL_CLAMP_TO_EDGE);

    // Modelos OBJ
    loadMeshes();

    // Quadric do corpo da cobra — normais suaves + UVs automáticas
    bodyQuad = gluNewQuadric();
    gluQuadricNormals(bodyQuad, GLU_SMOOTH);
    gluQuadricTexture(bodyQuad, GL_TRUE);
}

// =============================================================================
//  INICIALIZAÇÃO DO OpenGL
// =============================================================================

static void initGL() {
    glClearColor(0.08f, 0.08f, 0.18f, 1.0f);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Ilumina as duas faces de cada polígono — impede faces "viradas" dos
    // modelos OBJ de aparecerem completamente pretas.
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    // Renormaliza as normais após glScale — necessário com modelos escalados.
    glEnable(GL_NORMALIZE);

    // Ambiente ligeiramente mais alto (0.35 vs 0.28) para tirar o "quase preto"
    // das partes em sombra, sem afetar as cores iluminadas.
    GLfloat ambient[]  = { 0.35f, 0.35f, 0.35f, 1.0f };
    GLfloat diffuse[]  = { 0.90f, 0.88f, 0.80f, 1.0f };
    GLfloat specular[] = { 0.30f, 0.30f, 0.30f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    glShadeModel(GL_SMOOTH);
}

// =============================================================================
//  RESHAPE
// =============================================================================

static void reshape(int w, int h) {
    winW = w;
    winH = (h > 0) ? h : 1;
    glViewport(0, 0, winW, winH);
}

// =============================================================================
//  INPUT — teclado
// =============================================================================

static void keyboard(unsigned char key, int /*x*/, int /*y*/) {
    switch (key) {
    case 27:
        exit(0);
        break;

    case 'r': case 'R':
        initGame();
        glutTimerFunc((unsigned int)stepMs, timer, 0);
        break;

    case 'c': case 'C':
        camMode = (camMode + 1) % 3;
        break;

    case 'm': case 'M':
        toggleBGM();
        break;

    case 'w': case 'W': {
        Dir d; d.dx = 0; d.dz = -1;
        if (!d.isOpposite(curDir)) nextDir = d;
        break;
    }
    case 's': case 'S': {
        Dir d; d.dx = 0; d.dz = 1;
        if (!d.isOpposite(curDir)) nextDir = d;
        break;
    }
    case 'a': case 'A': {
        Dir d; d.dx = -1; d.dz = 0;
        if (!d.isOpposite(curDir)) nextDir = d;
        break;
    }
    case 'd': case 'D': {
        Dir d; d.dx = 1; d.dz = 0;
        if (!d.isOpposite(curDir)) nextDir = d;
        break;
    }
    }
    glutPostRedisplay();
}

// =============================================================================
//  INPUT — teclas especiais (setas)
// =============================================================================

static void specialKeys(int key, int /*x*/, int /*y*/) {
    Dir d; d.dx = 0; d.dz = 0;
    switch (key) {
    case GLUT_KEY_UP:    d.dx =  0; d.dz = -1; break;
    case GLUT_KEY_DOWN:  d.dx =  0; d.dz =  1; break;
    case GLUT_KEY_LEFT:  d.dx = -1; d.dz =  0; break;
    case GLUT_KEY_RIGHT: d.dx =  1; d.dz =  0; break;
    default: return;
    }
    if (!d.isOpposite(curDir)) nextDir = d;
}

// =============================================================================
//  INPUT — mouse (órbita da câmera de 3ª pessoa)
// =============================================================================

static void mouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        mouseDown  = (state == GLUT_DOWN);
        mouseLastX = x;
        mouseLastY = y;
    }
}

static void mouseMotion(int x, int y) {
    if (!mouseDown || camMode != 0) return;

    int dx = x - mouseLastX;
    int dy = y - mouseLastY;
    mouseLastX = x;
    mouseLastY = y;

    camAngleH += dx * 0.5f;
    camAngleV -= dy * 0.5f;

    if (camAngleV < 5.0f)  camAngleV = 5.0f;
    if (camAngleV > 89.0f) camAngleV = 89.0f;

    glutPostRedisplay();
}

// =============================================================================
//  MAIN
// =============================================================================

int main(int argc, char** argv) {
    srand((unsigned)time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Snake 3D — Trabalho Final de Computacao Grafica");

    initGL();
    loadTextures();   // carrega texturas reais + modelos OBJ
    initGame();
    startBGM();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);

    glutTimerFunc((unsigned int)stepMs, timer, 0);

    glutMainLoop();
    return 0;
}
