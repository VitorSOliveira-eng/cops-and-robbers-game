

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <raylib.h>


#define SCREEN_WIDTH    1024
#define SCREEN_HEIGHT   768
#define WORLD_WIDTH     5000
#define WORLD_HEIGHT    5000

#define BACKGROUND_COLOR      (Color){186, 149, 127, 255}
#define CAMERA_FOLLOW_THRESH  400

#define MAX_SKIDMARKS   500
#define SKIDMARK_TIME   3.0

//oleo
#define OIL_COUNT        15
#define OIL_SLOW_TIME     1.5f   
#define OIL_SLOW_FACTOR   0.25f  

#define ARQUIVO_RECORDE   "recorde.txt"



typedef struct {
    float x, y;
    int   width;
    int   length;

    float speed;
    float max_speed;
    float max_reverse_speed;  
    float speedup;
    float slowdown;

    float angle;
    float drift_angle;
    float drift_bias;

    float steering;
    float max_steering;
    float steering_speed;
    float steer_back_speed;

    float oil_slow_timer;    
} Carro;


typedef struct {
    float left_tire_x, left_tire_y;
    float right_tire_x, right_tire_y;
    double time;
} MarcaPneu;


typedef struct {
    Vector2 posicao;
    float   raio;
    bool    ativa;
} PocaDeOleo;



void InitGame(Carro *jogador, Carro *policia, PocaDeOleo pocas[OIL_COUNT],
              MarcaPneu marcas[MAX_SKIDMARKS], int *marca_count,
              bool *game_over, double *start_time);

void UpdateGame(Carro *jogador, Carro *policia, PocaDeOleo pocas[OIL_COUNT],
                 MarcaPneu marcas[MAX_SKIDMARKS], int *marca_count,
                 bool *game_over, double start_time, double *end_time,
                 float *recorde, Camera2D *camera, float dt);

void DrawGame(Carro *jogador, Carro *policia, PocaDeOleo pocas[OIL_COUNT],
              MarcaPneu marcas[MAX_SKIDMARKS], int marca_count,
              Texture2D chao_textura, Texture2D oleo_textura,
              Texture2D ladrao_textura, Texture2D policia_textura, Camera2D camera,
              bool game_over, double start_time, double end_time, float recorde);


static float CarregarRecorde(void);
static void  SalvarRecorde(float recorde);


static void AtualizarFisicaCarro(Carro *carro, float dt, int acelerar, int esterco);
static void LimitarNosLimitesDoMundo(Carro *carro);
static void AplicarEfeitoOleo(Carro *carro, PocaDeOleo pocas[OIL_COUNT], float dt);
static void RegistrarMarcaDePneu(Carro *carro, MarcaPneu marcas[MAX_SKIDMARKS], int *marca_count, bool drifting);
static void AtualizarCamera(Camera2D *camera, Carro *jogador);



int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Policia e Ladrao");
    SetTargetFPS(60);

    Image chao_imagem = LoadImage("craftpix-889156-free-racing-game-kit/PNG/Background_Tiles/Soil_Tile.png");
    ImageRotateCW(&chao_imagem);
    Texture2D chao_textura = LoadTextureFromImage(chao_imagem);
    UnloadImage(chao_imagem);


    Image oleo_imagem = LoadImage("craftpix-889156-free-racing-game-kit/PNG/Background_Tiles/oleo.png");
    Texture2D oleo_textura = LoadTextureFromImage(oleo_imagem);
    UnloadImage(oleo_imagem);

    Image ladrao_imagem = LoadImage("craftpix-889156-free-racing-game-kit/PNG/Car_1_Main_Positions/ladrao.png");
    Texture2D ladrao_textura = LoadTextureFromImage(ladrao_imagem);
    UnloadImage(ladrao_imagem);

    Image policia_imagem = LoadImage("craftpix-889156-free-racing-game-kit/PNG/Car_1_Main_Positions/policia.png");
    Texture2D policia_textura = LoadTextureFromImage(policia_imagem);
    UnloadImage(policia_imagem);

    Carro jogador = {0};
    Carro policia = {0};
    PocaDeOleo pocas[OIL_COUNT];
    MarcaPneu marcas[MAX_SKIDMARKS];
    int marca_count = 0;
    bool game_over = false;
    double start_time = 0;
    double end_time = 0;

    float recorde = CarregarRecorde();

    Camera2D camera = {
        .offset   = (Vector2){0, 0},
        .target   = (Vector2){0, 0},
        .rotation = 0,
        .zoom     = 1.0f,
    };

    InitGame(&jogador, &policia, pocas, marcas, &marca_count, &game_over, &start_time);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        UpdateGame(&jogador, &policia, pocas, marcas, &marca_count, &game_over,
                   start_time, &end_time, &recorde, &camera, dt);

        BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);

        DrawGame(&jogador, &policia, pocas, marcas, marca_count, chao_textura, oleo_textura,
                 ladrao_textura, policia_textura, camera, game_over, start_time, end_time, recorde);

        EndDrawing();

        if (game_over && IsKeyPressed(KEY_R)) {
            InitGame(&jogador, &policia, pocas, marcas, &marca_count, &game_over, &start_time);
        }
    }

    UnloadTexture(chao_textura);
    UnloadTexture(oleo_textura);
    UnloadTexture(ladrao_textura);
    UnloadTexture(policia_textura);
    CloseWindow();

    return 0;
}


void InitGame(Carro *jogador, Carro *policia, PocaDeOleo pocas[OIL_COUNT],
              MarcaPneu marcas[MAX_SKIDMARKS], int *marca_count,
              bool *game_over, double *start_time) {

    SetRandomSeed((unsigned int)time(NULL));

    //POSICAO INICIAL
    *jogador = (Carro){
        .x = WORLD_WIDTH / 2.0f,
        .y = WORLD_HEIGHT / 2.0f,
        .width  = 80,
        .length = 150,
        .speed = 0,
        .max_speed = 7.0f,
        .max_reverse_speed = 3.5f, 
        .speedup = 10.0f,
        .slowdown = 0.97f,
        .angle = 0,
        .drift_angle = 0,
        .drift_bias = 15.0f,
        .steering = 0,
        .max_steering = 4.0f,
        .steering_speed = 2.0f,
        .steer_back_speed = 0.04f,
        .oil_slow_timer = 0,
    };
    //POSICAO INICIAL POLICIA
    *policia = (Carro){
        .x = WORLD_WIDTH / 2.0f + 600,
        .y = WORLD_HEIGHT / 2.0f + 600,
        .width  = 80,
        .length = 150,
        .speed = 0,
        .max_speed = 7.6f,
        .max_reverse_speed = 3.8f,
        .speedup = 10.0f,
        .slowdown = 0.97f,
        .angle = 0,
        .drift_angle = 0,
        .drift_bias = 15.0f,
        .steering = 0,
        .max_steering = 4.0f,
        .steering_speed = 2.0f,
        .steer_back_speed = 0.04f,
        .oil_slow_timer = 0,
    };

    //POSICAO INICIAL POCAS OLEO
    for (int i = 0; i < OIL_COUNT; i++) {
        pocas[i].posicao.x = (float)GetRandomValue(200, WORLD_WIDTH - 200);
        pocas[i].posicao.y = (float)GetRandomValue(200, WORLD_HEIGHT - 200);
        pocas[i].raio = 40.0f;
        pocas[i].ativa = true;
    }

    for (int i = 0; i < MAX_SKIDMARKS; i++) {
        marcas[i] = (MarcaPneu){0};
    }
    *marca_count = 0;

    *game_over = false;
    *start_time = GetTime();
}



void UpdateGame(Carro *jogador, Carro *policia, PocaDeOleo pocas[OIL_COUNT],
                 MarcaPneu marcas[MAX_SKIDMARKS], int *marca_count,
                 bool *game_over, double start_time, double *end_time,
                 float *recorde, Camera2D *camera, float dt) {

    if (*game_over) {
        return;
    }

    int acelerar_jogador = 0;
    if (IsKeyDown(KEY_UP))        acelerar_jogador = 1;
    else if (IsKeyDown(KEY_DOWN)) acelerar_jogador = -1;

    int esterco_jogador = 0;
    if (IsKeyDown(KEY_LEFT))       esterco_jogador = -1;
    else if (IsKeyDown(KEY_RIGHT)) esterco_jogador = 1;

  
    AtualizarFisicaCarro(jogador, dt, acelerar_jogador, esterco_jogador);
    LimitarNosLimitesDoMundo(jogador);  

    bool drift_jogador = fabsf(jogador->drift_angle - jogador->angle) > 30.0f;
    RegistrarMarcaDePneu(jogador, marcas, marca_count, drift_jogador);
    
    //IA POLICIA

    float dx = jogador->x - policia->x;
    float dy = jogador->y - policia->y;
    float angulo_alvo = atan2f(dy, dx) * 180.0f / PI + 90.0f;


    float diff = fmodf(angulo_alvo - policia->angle + 540.0f, 360.0f) - 180.0f;

    int esterco_policia = 0;
    if (diff > 3.0f)       esterco_policia = 1;
    else if (diff < -3.0f) esterco_policia = -1;

    AtualizarFisicaCarro(policia, dt, 1, esterco_policia);

    AplicarEfeitoOleo(policia, pocas, dt);

    LimitarNosLimitesDoMundo(policia);   

    bool drift_policia = fabsf(policia->drift_angle - policia->angle) > 30.0f;
    RegistrarMarcaDePneu(policia, marcas, marca_count, drift_policia);


    //game over
    float dist_x = jogador->x - policia->x;
    float dist_y = jogador->y - policia->y;
    float distancia = sqrtf(dist_x * dist_x + dist_y * dist_y);
   
    float raio_colisao = (jogador->length + policia->length) / 4.0f;

    if (distancia < raio_colisao) {
        *game_over = true;
        *end_time = GetTime();
        //recorde
        float tempo_sobrevivencia = (float)(*end_time - start_time);
        if (tempo_sobrevivencia > *recorde) {
            *recorde = tempo_sobrevivencia;
            SalvarRecorde(*recorde);
        }
    }

    AtualizarCamera(camera, jogador);
}

//PARTE DE RENDERIZACAO 
void DrawGame(Carro *jogador, Carro *policia, PocaDeOleo pocas[OIL_COUNT],
              MarcaPneu marcas[MAX_SKIDMARKS], int marca_count,
              Texture2D chao_textura, Texture2D oleo_textura,
              Texture2D ladrao_textura, Texture2D policia_textura, Camera2D camera,
              bool game_over, double start_time, double end_time, float recorde) {


    BeginMode2D(camera);

    int colunas = (int)ceilf((float)WORLD_WIDTH / chao_textura.width);
    int linhas  = (int)ceilf((float)WORLD_HEIGHT / chao_textura.height);
    for (int x = 0; x < colunas; x++) {
        for (int y = 0; y < linhas; y++) {
            DrawTexture(chao_textura, x * chao_textura.width, y * chao_textura.height, WHITE);
        }
    }

    for (int i = 0; i < OIL_COUNT; i++) {
        if (pocas[i].ativa) {
            Rectangle origem_textura = { 0, 0, (float)oleo_textura.width, (float)oleo_textura.height };
            float diametro = pocas[i].raio * 2.0f;
            Rectangle destino = {
                pocas[i].posicao.x, pocas[i].posicao.y,
                diametro, diametro
            };
            Vector2 origem_destino = { diametro / 2.0f, diametro / 2.0f };
            DrawTexturePro(oleo_textura, origem_textura, destino, origem_destino, 0.0f, WHITE);
        }
    }

    double agora = GetTime();
    for (int i = 0; i < marca_count && i < MAX_SKIDMARKS; i++) {
        MarcaPneu m = marcas[i];
        if (agora - m.time > SKIDMARK_TIME) continue;
        DrawCircle(m.left_tire_x, m.left_tire_y, 6, BLACK);
        DrawCircle(m.right_tire_x, m.right_tire_y, 6, BLACK);
    }

    Rectangle ladrao_origem_textura = { 0, 0, (float)ladrao_textura.width, (float)ladrao_textura.height };
    Rectangle jogador_rec = { jogador->x, jogador->y, (float)jogador->width, (float)jogador->length };
    Vector2 jogador_origem = { jogador->width / 2.0f, jogador->length / 2.0f };
    DrawTexturePro(ladrao_textura, ladrao_origem_textura, jogador_rec, jogador_origem, jogador->angle, WHITE);

    Rectangle policia_origem_textura = { 0, 0, (float)policia_textura.width, (float)policia_textura.height };
    Rectangle policia_rec = { policia->x, policia->y, (float)policia->width, (float)policia->length };
    Vector2 policia_origem = { policia->width / 2.0f, policia->length / 2.0f };
    DrawTexturePro(policia_textura, policia_origem_textura, policia_rec, policia_origem, policia->angle, WHITE);

    EndMode2D();

    double tempo_sobrevivencia = game_over ? (end_time - start_time) : (GetTime() - start_time);
    DrawText(TextFormat("Tempo: %.1f s", tempo_sobrevivencia), 20, 20, 24, BLACK);

    DrawText(TextFormat("Recorde: %.1f s", recorde), 20, 50, 24, BLACK);

    if (game_over) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 150});

        const char *msg = "VOCE FOI PEGO!";
        int msg_width = MeasureText(msg, 50);
        DrawText(msg, SCREEN_WIDTH / 2 - msg_width / 2, SCREEN_HEIGHT / 2 - 60, 50, WHITE);

    
        if (tempo_sobrevivencia >= recorde && tempo_sobrevivencia > 0.0) {
            const char *msg_recorde = "NOVO RECORDE!";
            int msg_recorde_width = MeasureText(msg_recorde, 28);
            DrawText(msg_recorde, SCREEN_WIDTH / 2 - msg_recorde_width / 2, SCREEN_HEIGHT / 2 - 10, 28, YELLOW);
        }

        const char *msg2 = "Pressione R para reiniciar";
        int msg2_width = MeasureText(msg2, 24);
        DrawText(msg2, SCREEN_WIDTH / 2 - msg2_width / 2, SCREEN_HEIGHT / 2 + 30, 24, WHITE);
    }
}

static void AtualizarFisicaCarro(Carro *carro, float dt, int acelerar, int esterco) {
    if (acelerar == 1) {
        carro->speed += carro->speedup * dt;
        if (carro->speed > carro->max_speed) {
            carro->speed = carro->max_speed;
        }
    } else if (acelerar == -1) {

        carro->speed -= carro->speedup * dt;
        if (carro->speed < -carro->max_reverse_speed) {
            carro->speed = -carro->max_reverse_speed;
        }
    } else {
        carro->speed *= carro->slowdown;
    }

    if (esterco == -1) {
        carro->steering -= carro->steering_speed * dt * fabsf(carro->speed);
        if (carro->steering < -carro->max_steering) carro->steering = -carro->max_steering;
    } else if (esterco == 1) {
        carro->steering += carro->steering_speed * dt * fabsf(carro->speed);
        if (carro->steering > carro->max_steering) carro->steering = carro->max_steering;
    }

    carro->steering *= (1.0f - carro->steer_back_speed);
    carro->angle += carro->steering;

    carro->drift_angle = (carro->angle + carro->drift_angle * carro->drift_bias) / (1.0f + carro->drift_bias);

    float radianos = PI * (carro->angle - 90.0f) / 180.0f;
    carro->x += carro->speed * cosf(radianos);
    carro->y += carro->speed * sinf(radianos);

    radianos = PI * (carro->drift_angle - 90.0f) / 180.0f;
    carro->x += carro->speed * cosf(radianos);
    carro->y += carro->speed * sinf(radianos);
}

static void LimitarNosLimitesDoMundo(Carro *carro) {
    if (carro->x < 0) carro->x = 0;
    if (carro->x > WORLD_WIDTH) carro->x = WORLD_WIDTH;
    if (carro->y < 0) carro->y = 0;
    if (carro->y > WORLD_HEIGHT) carro->y = WORLD_HEIGHT;
}

static void AplicarEfeitoOleo(Carro *carro, PocaDeOleo pocas[OIL_COUNT], float dt) {
    for (int i = 0; i < OIL_COUNT; i++) {
        if (!pocas[i].ativa) continue;

        float dx = carro->x - pocas[i].posicao.x;
        float dy = carro->y - pocas[i].posicao.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < pocas[i].raio) {
            carro->oil_slow_timer = OIL_SLOW_TIME;
        }
    }

    if (carro->oil_slow_timer > 0.0f) {
        carro->oil_slow_timer -= dt;

        float velocidade_maxima_temporaria = carro->max_speed * OIL_SLOW_FACTOR;
        if (carro->speed > velocidade_maxima_temporaria) {
            carro->speed = velocidade_maxima_temporaria;
        }
    }
}

static void RegistrarMarcaDePneu(Carro *carro, MarcaPneu marcas[MAX_SKIDMARKS], int *marca_count, bool drifting) {
    if (!drifting) return;

    float radianos = PI * (carro->angle - 240.0f) / 180.0f;
    float left_tire_x = carro->x + (carro->length / 2.6f) * cosf(radianos);
    float left_tire_y = carro->y + (carro->length / 2.6f) * sinf(radianos);

    radianos = PI * (carro->angle - 300.0f) / 180.0f;
    float right_tire_x = carro->x + (carro->length / 2.6f) * cosf(radianos);
    float right_tire_y = carro->y + (carro->length / 2.6f) * sinf(radianos);

    marcas[*marca_count % MAX_SKIDMARKS] = (MarcaPneu){
        left_tire_x, left_tire_y, right_tire_x, right_tire_y, GetTime()
    };
    (*marca_count)++;
}

static void AtualizarCamera(Camera2D *camera, Carro *jogador) {
    if (jogador->x + camera->offset.x < CAMERA_FOLLOW_THRESH)
        camera->offset.x = -jogador->x + CAMERA_FOLLOW_THRESH;

    if (jogador->x + camera->offset.x > (SCREEN_WIDTH - CAMERA_FOLLOW_THRESH))
        camera->offset.x = -jogador->x + (SCREEN_WIDTH - CAMERA_FOLLOW_THRESH);

    if (jogador->y + camera->offset.y < CAMERA_FOLLOW_THRESH)
        camera->offset.y = -jogador->y + CAMERA_FOLLOW_THRESH;

    if (jogador->y + camera->offset.y > (SCREEN_HEIGHT - CAMERA_FOLLOW_THRESH))
        camera->offset.y = -jogador->y + (SCREEN_HEIGHT - CAMERA_FOLLOW_THRESH);

    if (camera->offset.x > 0) camera->offset.x = 0;
    if (camera->offset.x < -WORLD_WIDTH + SCREEN_WIDTH + CAMERA_FOLLOW_THRESH)
        camera->offset.x = -WORLD_WIDTH + SCREEN_WIDTH + CAMERA_FOLLOW_THRESH;

    if (camera->offset.y > 0) camera->offset.y = 0;
    if (camera->offset.y < -WORLD_HEIGHT + SCREEN_HEIGHT + CAMERA_FOLLOW_THRESH)
        camera->offset.y = -WORLD_HEIGHT + SCREEN_HEIGHT + CAMERA_FOLLOW_THRESH;
}

static float CarregarRecorde(void) {
    float recorde = 0.0f;

    FILE *arquivo = fopen(ARQUIVO_RECORDE, "r");
    if (arquivo != NULL) {
        fscanf(arquivo, "%f", &recorde);
        fclose(arquivo);
    }

    return recorde;
}

static void SalvarRecorde(float recorde) {
    FILE *arquivo = fopen(ARQUIVO_RECORDE, "w");
    if (arquivo != NULL) {
        fprintf(arquivo, "%.2f", recorde);
        fclose(arquivo);
    }
}
