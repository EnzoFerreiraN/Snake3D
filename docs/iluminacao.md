# Iluminação

## Modelo utilizado

O jogo usa o **modelo de reflexão de Phong** (cálculo de componentes ambiente + difuso + especular), avaliado **por vértice** com o método de shading de **Gouraud**. A implementação é feita inteiramente pelo **pipeline fixed-function do OpenGL**, sem shaders GLSL — toda a configuração passa pelas funções clássicas `glLight*`, `glMaterial*` / `glColorMaterial` e `glShadeModel`.

---

## Conceito: Gouraud shading e suas alternativas

O problema de **shading** (sombreamento) é decidir qual cor atribuir a cada pixel de uma superfície iluminada. Existem três abordagens clássicas, em ordem crescente de qualidade e custo:

### Flat shading (`GL_FLAT`)
Uma única cor é calculada para toda a face (geralmente usando a normal da face inteira). O resultado são polígonos com tonalidade uniforme e bordas claramente visíveis — adequado para objetos com geometria intencionalmente facetada, mas artificial em superfícies curvas.

### Gouraud shading (`GL_SMOOTH`) — **método utilizado no jogo**
A cor é calculada **apenas nos vértices** do polígono (usando as normais de vértice). O rasterizador então **interpola linearmente** essas cores pelos pixels intermediários. O resultado é uma transição suave entre vértices sem o custo de iluminar cada pixel individualmente.

```
Vértice A → cor_A
Vértice B → cor_B        pixel_P = lerp(cor_A, cor_B, t)
Vértice C → cor_C
```

**Por que Gouraud é adequado aqui?** O corpo da cobra é formado por esferas e cilindros GLU com 12–14 subdivisões, o que cria malhas com vértices suficientemente próximos para que a interpolação produza um aspecto suave. O custo é proporcional ao número de vértices, não de pixels.

**Limitação conhecida:** reflexos especulares (highlights) calculados nos vértices podem "desaparecer" quando o ponto de maior brilho cai no interior de um triângulo grande, longe de qualquer vértice. Para objetos grandes e lisos isso é perceptível.

### Phong shading per-pixel (não utilizado)
A normal é interpolada por pixel e a equação de iluminação é avaliada para cada pixel. Produz highlights especulares precisos mesmo em triângulos grandes. Exige **shaders GLSL** (vertex + fragment shader), que **não estão presentes neste projeto** — o projeto usa exclusivamente o pipeline fixed-function.

### Resumo comparativo

| Método | Cálculo da cor | Qualidade | Custo | Usado no jogo? |
|---|---|---|---|---|
| Flat shading | Por face | Baixa | Muito baixo | Não |
| **Gouraud shading** | **Por vértice + interpolação** | **Média** | **Baixo** | **Sim** |
| Phong per-pixel | Por pixel | Alta | Alto (GPU shader) | Não |

---

## Configuração (`initGL`)

A iluminação é inicializada uma vez na função `initGL`, antes da primeira renderização:

### Habilitação

```cpp
glEnable(GL_LIGHTING);       // ativa o sistema de iluminação
glEnable(GL_LIGHT0);         // usa a luz 0 (único ponto/direcional da cena)
glEnable(GL_COLOR_MATERIAL); // glColor() define a componente difusa/ambiente
glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
```

`GL_COLOR_MATERIAL` elimina a necessidade de chamar `glMaterialfv` para cada objeto: simplesmente chamar `glColor3f(r, g, b)` antes de desenhar define a cor de difusão e ambiente daquele objeto, mantendo o código de renderização mais simples.

### Shading suave (Gouraud)

```cpp
glShadeModel(GL_SMOOTH); // interpolação de Gouraud entre vértices
```

No modo `GL_SMOOTH`, a cor calculada em cada vértice é interpolada linearmente para os pixels intermediários, produzindo superfícies curvas com aspecto suave.

### Normalização automática

```cpp
glEnable(GL_NORMALIZE);
```

Quando `glScale` é usado (ou quando objetos são escalados pela normalização do OBJ loader), as normais dos vértices podem perder a magnitude unitária. `GL_NORMALIZE` re-normaliza automaticamente as normais antes do cálculo de iluminação, evitando que faces fiquem superiluminadas ou subiluminadas.

### Iluminação dos dois lados

```cpp
glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
```

Modelos OBJ com faces orientadas de forma inconsistente (normais apontando para dentro em algumas faces) apareceriam completamente pretas sem isso. Com `TWO_SIDE`, o OpenGL ilumina tanto a face da frente (`GL_FRONT`) quanto a de trás (`GL_BACK`) de cada polígono, usando a mesma equação de iluminação.

---

## Componentes da luz — modelo de Phong

O **modelo de reflexão de Phong** decompõe a luz em três componentes independentes. A `GL_LIGHT0` tem as três configuradas:

```cpp
GLfloat ambient[]  = { 0.35f, 0.35f, 0.35f, 1.0f }; // componente ambiente
GLfloat diffuse[]  = { 0.90f, 0.88f, 0.80f, 1.0f }; // componente difusa (tom quente)
GLfloat specular[] = { 0.30f, 0.30f, 0.30f, 1.0f }; // componente especular (reflexo)
```

| Componente | O que representa | Comportamento |
|---|---|---|
| **Ambiente** | Luz indireta difusa (espalhada pelo ambiente) | Constante, não depende do ângulo de visão nem da posição da luz |
| **Difusa** | Reflexão da luz direta na superfície opaca | Proporcional a `max(N·L, 0)` — máxima quando a superfície é perpendicular à luz |
| **Especular** | Reflexo "brilhante" da fonte de luz | Proporcional a `max(N·H, 0)^shininess` — concentrado num ponto conforme `shininess` sobe |

- **Ambiente (0.35)**: nível relativamente alto para evitar que as áreas em sombra fiquem "quase pretas", preservando legibilidade visual em todas as partes do grid.
- **Difusa (tom quente)**: a ligeira redução no azul (0.80 vs. 0.90) dá um tom levemente aquecido à cena, mais agradável visualmente do que branco puro.
- **Especular (0.30)**: reflexo moderado — visível em superfícies mais lisas como as esferas do corpo da cobra, mas sem brilho excessivo.

---

## Posição da luz

```cpp
GLfloat lightPos[] = { GRID * 0.25f, GRID * 1.8f, GRID * 0.25f, 0.0f };
glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
```

O último componente `w = 0.0` define uma **luz direcional** (fonte no infinito), não pontual. A direção é calculada como o vetor da origem até `(x, y, z)`, ou seja, a luz vem de cima e levemente à frente-esquerda do grid.

A posição é definida a cada frame em `display()`, após configurar a câmera — isso garante que a posição da luz seja transformada pelo espaço da câmera atual, o que é o comportamento correto para uma luz "do mundo" (não vinculada ao olho).

---

## Combinação textura × iluminação (`GL_MODULATE`)

Como nenhuma chamada a `glTexEnv` é feita no projeto, o modo de combinação de textura é o padrão do OpenGL: **`GL_MODULATE`**.

Nesse modo, a cor final do pixel é o produto da cor calculada pela iluminação (Gouraud) com a cor do texel amostrado:

```
cor_pixel = cor_gouraud × cor_textura
```

O efeito é que a textura "recebe" a iluminação: partes iluminadas ficam com as cores da textura normais, e partes em sombra ficam com cores escurecidas proporcionalmente. Isso é o comportamento esperado para objetos opacos com textura no mundo 3D.

A alternativa `GL_REPLACE` ignoraria completamente a iluminação e exibiria a textura "crua" — usada no skybox via `glDisable(GL_LIGHTING)`, não por `glTexEnv`.

---

## Onde a iluminação é desativada

Alguns elementos da cena precisam de cores puras, sem sombreamento:

| Elemento | Como é desativado |
|---|---|
| **Skybox** | `glDisable(GL_LIGHTING)` antes de desenhar o cubo de ambiente |
| **Borda do grid** (linha verde) | `glDisable(GL_LIGHTING)` explícito em `drawFloor` |
| **Portais** | `glDisable(GL_LIGHTING)` dentro de `glPushAttrib / glPopAttrib` |
| **HUD 2D** | `glDisable(GL_LIGHTING)` antes do modo ortográfico; reativado depois |

O padrão `glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | ...)` / `glPopAttrib()` nos portais salva e restaura automaticamente o estado de iluminação, sem precisar reativar manualmente cada flag.

---

## Resumo visual — equação de Phong avaliada por vértice (Gouraud)

```
Modelo de reflexão de Phong, calculado em cada vértice (Gouraud shading):

  cor_vértice = ambiente_global
              + Σ [ ambiente_luz
                  + difusa_luz  × max(N·L, 0)
                  + especular_luz × max(N·H, 0)^shininess ]

  N = normal do vértice (re-normalizada por GL_NORMALIZE)
  L = vetor unitário para a fonte de luz direcional
  H = vetor half-way = normalize(L + olho)   (Blinn-Phong half vector)

  Entre vértices → interpolação linear de Gouraud pelo rasterizador.

  Cor final do pixel = cor_vértice_interpolada × cor_textura   (GL_MODULATE)
```

Com `GL_COLOR_MATERIAL`, a `cor_material` (difusa + ambiente) vem diretamente de `glColor3f`, simplificando o código de renderização de cada objeto.

---

## Bibliotecas utilizadas

| Biblioteca | Papel na iluminação |
|---|---|
| **OpenGL fixed-function** (`opengl32`) | Toda a API de iluminação: `glLight*`, `glColorMaterial`, `glShadeModel`, `glEnable(GL_LIGHTING)` |
| **GLU** (`glu32`) | Fornece normais suaves para os quadrics (`gluQuadricNormals(bodyQuad, GLU_SMOOTH)`), usadas no cálculo de Gouraud do corpo da cobra |
| **GLUT** (`glut32`) | Geometrias prontas com normais pré-calculadas: `glutSolidSphere`, `glutSolidTorus`, etc. — compatíveis com o sistema de iluminação do OpenGL |
