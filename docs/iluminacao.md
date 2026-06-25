# Iluminação

## Modelo utilizado

O jogo usa o modelo de iluminação **fixed-function do OpenGL** com shading de **Gouraud** — as cores são calculadas nos vértices e interpoladas entre eles pelo rasterizador. Não há shaders GLSL; toda a iluminação é feita com as funções clássicas `glLight*` e `glMaterial*` / `glColorMaterial`.

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

### Shading suave

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

## Componentes da luz

A `GL_LIGHT0` tem três componentes de cor configuradas:

```cpp
GLfloat ambient[]  = { 0.35f, 0.35f, 0.35f, 1.0f }; // luz ambiente
GLfloat diffuse[]  = { 0.90f, 0.88f, 0.80f, 1.0f }; // luz difusa (tom quente)
GLfloat specular[] = { 0.30f, 0.30f, 0.30f, 1.0f }; // especular (reflexo)
```

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

## Resumo visual

```
Equação de Gouraud (por vértice):

  cor_final = ambiente_global
            + Σ [ ambiente_luz + difusa_luz * max(N·L, 0) + especular_luz * max(N·H, 0)^shininess ]

  N = normal do vértice (normalizada automaticamente)
  L = vetor para a fonte de luz
  H = vetor half-way entre L e o olho
```

Com `GL_COLOR_MATERIAL`, a `cor_material` (difusa + ambiente) vem diretamente de `glColor3f`, simplificando o código de renderização de cada objeto.
