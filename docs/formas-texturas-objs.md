# Formas, Texturas e Modelos OBJ

## Formas e primitivas

### Cubo texturizado manual

A função `drawCubeTex(sx, sy, sz)` desenha um cubo de dimensões arbitrárias usando `glBegin(GL_QUADS)`. Para cada uma das seis faces, são especificadas normais (`glNormal3f`), coordenadas de textura (`glTexCoord2f`) e vértices (`glVertex3f`). Esse cubo é utilizado como:

- **Paredes** do grid central — pilares de `CELL * 0.95 × 2.0 × 0.95`
- **Fallback da cabeça** (amarelo sólido) quando o modelo OBJ não carrega
- **Fallback do corpo** quando o modelo não está disponível
- **Fallback da comida** (vermelho) quando o modelo da maçã não carrega

### Corpo da cobra — tubo com quadrics GLU

O corpo é renderizado como um **tubo orgânico contínuo**, combinando esferas nas juntas e cilindros nas ligações, todos criados com `GLUquadric`:

```
Cabeça → [cilindro] → esfera → [cilindro] → esfera → ... → esfera (cauda)
```

**Esferas** (`gluSphere`) são desenhadas em cada segmento do corpo a partir do pescoço, cobrindo as curvas e dando um aspecto suave às dobras. **Cilindros** (`gluCylinder`) conectam pares de segmentos consecutivos, incluindo a ligação cabeça-pescoço.

Ambos usam 14 slices/12 stacks (esferas) e 14 slices (cilindros) para qualidade razoável sem custo excessivo.

O **afinamento** pescoço→cauda é calculado por `bodyRadiusAt(i, n)`:

```cpp
float rNeck = CELL * 0.42f;  // gordo no pescoço
float rTail = CELL * 0.16f;  // fino na cauda
float t = (float)(i - 1) / (float)(n - 1);  // 0 = pescoço, 1 = cauda
return rNeck + (rTail - rNeck) * t;          // interpolação linear
```

Cada cilindro recebe o raio do segmento de origem em uma ponta e o do segmento de destino na outra, criando a variação de espessura ao longo do corpo.

Para orientar os cilindros na direção correta (cada par de segmentos pode estar em direções diferentes), o ângulo `yawDeg` é calculado com `atan2(dx, dz)` e aplicado com `glRotatef` em torno do eixo Y, já que `gluCylinder` aponta no eixo +Z local.

**O quadric** (`bodyQuad`) é criado uma única vez em `loadTextures` com normais suaves (`GLU_SMOOTH`) e geração automática de coordenadas UV (`GLU_TRUE`), permitindo que a textura de escamas seja mapeada automaticamente.

### Cabeça da cobra — modelo OBJ orientado

A cabeça usa o modelo `snake_head.obj` texturizado. Para orientá-la na direção de movimento, é aplicada uma rotação em Y com `atan2(-curDir.dz, curDir.dx)` (o modelo OBJ tem seu eixo principal em +X).

### Portais — efeito visual composto

Cada portal é uma composição de elementos animados, todos usando `glutGet(GLUT_ELAPSED_TIME)` para animar com base no tempo real:

| Elemento | Função GL | Descrição |
|---|---|---|
| Núcleo pulsante | `glutSolidSphere` | Raio oscila com `sin(t * 3)`, gira em Y |
| Esfera wireframe | `glutWireSphere` | Gira em direção arbitrária (0.3, 1, 0.2) |
| Dois anéis (torus) | `glutSolidTorus` | Giram em sentidos opostos |
| Partículas orbitando | `GL_POINTS` com `glPointSize(4)` | 24 pontos em órbita elíptica animada |

Todo o efeito usa **blending aditivo** (`GL_SRC_ALPHA, GL_ONE`) com `glDepthMask(GL_FALSE)`, criando o brilho transparente sem obscurecer objetos atrás do portal.

### Skybox — cubo de ambiente

O skybox é um cubo de lado 400 (`R = 200`) com 6 quads texturizados, centrado dinamicamente na posição do olho para que nunca pareça se afastar. É renderizado com:

- `glDisable(GL_DEPTH_TEST)` — o skybox é sempre desenhado "atrás" de tudo
- `glDepthMask(GL_FALSE)` — não escreve no Z-buffer
- `glDisable(GL_LIGHTING)` — cores puras da textura, sem sombreamento

A posição do olho é extraída da matriz modelview atual (inversa de R^T · t), o que garante funcionamento correto em qualquer modo de câmera.

---

## Texturas

### Carregamento com stb_image (`loadTexture`)

A função `loadTexture(path, fallback, wrap, mipmap)` tenta carregar a imagem com `stbi_load`. Se o arquivo existir, cria uma textura OpenGL com:

- `gluBuild2DMipmaps` + `GL_LINEAR_MIPMAP_LINEAR` (padrão, `mipmap=true`) — para texturas repetidas com redução suave à distância
- `glTexImage2D` + `GL_LINEAR` sem mipmap (`mipmap=false`) — para a textura da maçã, que tem fundo preto: evita que níveis menores do mipmap "vazem" o preto para as bordas

O parâmetro `wrap` define `GL_REPEAT` (para texturas do cenário que se repetem) ou `GL_CLAMP_TO_EDGE` (para as 6 faces do skybox, eliminando costuras nas bordas).

Se o arquivo não existir ou falhar no carregamento, `makeProceduralTex` é chamado como fallback.

### Texturas procedurais (fallback)

`makeProceduralTex(kind)` gera uma textura 64×64 na CPU, sem precisar de arquivo em disco. Cada tipo (`TexKind`) gera um padrão diferente:

| Kind | Padrão | Uso |
|---|---|---|
| `TK_CHECKER` | Xadrez verde | Chão (fallback) |
| `TK_BRICK` | Tijolos com rejunte | Parede (fallback) |
| `TK_SCALES` | Escamas xadrez verde-azul | Corpo da cobra (fallback) |
| `TK_APPLE` | Gradiente esférico vermelho | Maçã (fallback) |
| `TK_HEAD` | Amarelo sólido | Cabeça (fallback) |
| `TK_SKY` | Gradiente azul vertical | Skybox (fallback) |

### Mapeamento de texturas no jogo

| Textura | Arquivo | Aplicada em |
|---|---|---|
| `texFloor` | `Grass005_1K-JPG_Color.jpg` | Chão (repetição 4×) |
| `texBody` | `Stylized_Scales_003_basecolor.png` | Corpo e cabeça da cobra |
| `texWall` | `PavingStones138_1K-JPG_Color.jpg` | Parede central |
| `texFood` | `food_apple_01_diff_4k.jpg` | Modelo da maçã (sem mipmap) |
| `texSky[0..5]` | `skybox/posx/negx/posy/negy/posz/negz.jpg` | 6 faces do skybox |

---

## Modelos OBJ

### Loader próprio (`loadOBJ`)

O jogo não usa nenhuma biblioteca de carregamento de modelos — o parser OBJ é implementado do zero. A função `loadOBJ(path, mesh, targetSize)` faz:

**1. Parse linha a linha:**
- `v x y z` → vértices (posição)
- `vt u v` → coordenadas de textura UV
- `vn x y z` → normais de vértice
- `f v/t/n ...` → faces (polígonos)

A função `parseFaceVert` trata os tokens de face manualmente para suportar os formatos `v`, `v/t`, `v//n` e `v/t/n`.

**2. Normalização por bounding box:**

Após o parse, o loader calcula a bounding box (min/max em X, Y, Z) e a dimensão máxima. O fator de escala é `targetSize / maior_dimensão`, e o centro é subtraído antes de escalar:

```cpp
glVertex3f((vx[vi] - cx) * scale,
           (vy[vi] - cy) * scale,
           (vz[vi] - cz) * scale);
```

Isso garante que o modelo caiba exatamente em `targetSize` independentemente da escala original do arquivo.

**3. Triangulação em leque:**

Faces com mais de 3 vértices (quads, ngons) são trianguladas em leque: `(0,1,2), (0,2,3), (0,3,4), ...`

**4. Compilação em display list:**

Toda a geometria é compilada em uma **display list OpenGL** (`glNewList`/`glEndList`). Na renderização, apenas `glCallList(mesh.list)` é chamado, sem reprocessar os vértices a cada frame.

### Modelos carregados

| Arquivo | `targetSize` | Uso |
|---|---|---|
| `textures/apple/food_apple_01_4k.obj` | `CELL * 0.70` | Maçã flutuante e giratória |
| `textures/snake/snake_head.obj` | `CELL * 2.10` | Cabeça da cobra |

Se qualquer modelo falhar no carregamento (`mesh.list == 0`), o jogo usa automaticamente o cubo-fallback correspondente, sem travar.
