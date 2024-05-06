/**
 * Classic game mode:
 * - incremental harder levels
 * - random generated levels
 * - nice and simple visuals
 * - simple sound effects
 * - the difficult is a mix of movement speed and obstacle avoidance
 * - show level number
 * - infinite levels??
 * - online ranking??
 * - next level unlock after x amount of scores (fruits??)
 * - next level door is through an opening on a side of the grid
*/
#include "raylib.h"
#include "raymath.h"
#include "stdio.h"
#include "float.h"
#include "stdlib.h"

#define GAME_SNAKE_TAIL_MAX_LENGTH 64
#define GAME_MAX_ITEMS 16

typedef struct Timer
{
    double elapsedTime;
    double previousTime;
} Timer;

typedef enum TileValue
{
    TILE_EMPTY,
    TILE_WALL,
    TILE_PLAYER,
} TileValue;

typedef struct TilePosition
{
    int row;
    int col;
} TilePosition;


typedef enum ItemType
{
    ITEM_NONE,
    ITEM_APPLE,
} ItemType;

typedef struct Item
{
    ItemType type;
    TilePosition tilePosition;
    Vector2 position;
    float width;
    float height;
    double lifeTime;
    double spawnTime;
    double spawnElapsedTime;
    int scorePoints;
} Item;

typedef struct TileMap
{
    int rows;
    int cols;
    TileValue *tiles;
    float tileWidth;
    float tileHeight;
} TileMap;

typedef struct SnakeTail
{
    TilePosition tilePosition;
    Vector2 position;
    float width;
    float height;
} SnakeTail;

typedef struct Snake
{
    TilePosition tilePosition;
    Vector2 position;
    Vector2 direction;
    Vector2 lookDirection;

    float headWidth;
    float headHeight;

    SnakeTail tail[GAME_SNAKE_TAIL_MAX_LENGTH];
    int tailLength;

    float speed; // Tiles per second
    Timer moveTimer;

    bool hasMove;
} Snake;

typedef struct Game
{
    int viewportWidth;
    int viewportHeight;

    TileMap tileMap;
    Snake snake;

    Item items[GAME_MAX_ITEMS];

    Sound eatSound;

    int score;
    int appleSpawnCount;
    float appleSpawnRate;
    double appleLastDespawnTime;
    bool isPaused;
} Game;

bool GameIsTileValid(TileMap *tileMap, TilePosition tilePosition) {
    return tilePosition.row >= 0 && tilePosition.row < tileMap->rows && tilePosition.col >= 0 && tilePosition.col < tileMap->cols;
}

int GameGetTileValue(TileMap *tileMap, TilePosition tilePosition) {
    return tileMap->tiles[tilePosition.row * tileMap->cols + tilePosition.col];
}

void GameSetTileValue(TileMap *tileMap, TilePosition tilePosition, TileValue value) {
    tileMap->tiles[tilePosition.row * tileMap->cols + tilePosition.col] = value;
}

void GameGetTileFromVector2(TileMap *tileMap, Vector2 pixelPosition, TilePosition *tilePosition) {
    tilePosition->row = pixelPosition.y / tileMap->tileHeight;
    tilePosition->col = pixelPosition.x / tileMap->tileWidth;
}

void GameGetTilePositionFromDirection(Vector2 direction, TilePosition originTilePosition, TilePosition *tilePosition) {
    tilePosition->row = direction.y + originTilePosition.row;
    tilePosition->col = direction.x + originTilePosition.col;
}

void GameGetMouseTile(TileMap *tileMap, TilePosition *tilePosition) {
    GameGetTileFromVector2(tileMap, GetMousePosition(), tilePosition);
}

bool GameIsTilePositionEqual(TilePosition tilePositionA, TilePosition tilePositionB) {
    return tilePositionA.row == tilePositionB.row && tilePositionA.col == tilePositionB.col;
}

bool GameIsSnakeAtTile(Snake *snake, TilePosition tilePosition) {
    if (GameIsTilePositionEqual(snake->tilePosition, tilePosition)) {
        return true;
    }

    for (int i = 0; i < snake->tailLength; i++) {
        if (GameIsTilePositionEqual(snake->tail[i].tilePosition, tilePosition)) {
            return true;
        }
    }

    return false;
}

bool GameIsItemAtTile(Game *game, TilePosition tilePosition) {
    for (int i = 0; i < GAME_MAX_ITEMS; i++) {
        if( GameIsTilePositionEqual(game->items[i].tilePosition, tilePosition)) {
            return true;
        }
    }

    return false;
}

bool GameGetRandomEmptyTile(Game *game, TilePosition *tilePosition) {
    int attemp = 0;
    while (attemp++ < 10000) {
        TilePosition newTilePosition;
        newTilePosition.row = GetRandomValue(0, game->tileMap.rows - 1);
        newTilePosition.col = GetRandomValue(0, game->tileMap.cols - 1);
        TileValue tileValue = GameGetTileValue(&game->tileMap, newTilePosition);

        if (tileValue != TILE_WALL && !GameIsSnakeAtTile(&game->snake, newTilePosition) && !GameIsItemAtTile(game, newTilePosition)) {
            tilePosition->row = newTilePosition.row;
            tilePosition->col = newTilePosition.col;

            return true;
        }
    }

    return false;
}

Item* GameCheckSnakeHitsItem(Game *game, Snake *snake) {
    for (int i = 0; i < GAME_MAX_ITEMS; i++) {
        Item *item = &game->items[i];

        if (item->type != ITEM_NONE) {
            if (GameIsTilePositionEqual(snake->tilePosition, item->tilePosition)) {
                return item;
            }
        }
    }

    return NULL;
}

void GameGrowSnake(Snake *snake) {
    snake->tail[snake->tailLength].position.x = snake->tail[snake->tailLength - 1].position.x;
    snake->tail[snake->tailLength].position.y = snake->tail[snake->tailLength - 1].position.y;
    snake->tail[snake->tailLength].tilePosition.row = snake->tail[snake->tailLength - 1].tilePosition.row;
    snake->tail[snake->tailLength].tilePosition.col = snake->tail[snake->tailLength - 1].tilePosition.col;
    snake->tail[snake->tailLength].width = snake->tail[snake->tailLength - 1].width;
    snake->tail[snake->tailLength].height = snake->tail[snake->tailLength - 1].height;
    snake->tailLength += 1;

    printf("Snake grew %d\n", snake->tailLength);
}

void GameMoveSnake(TileMap *tileMap, Snake *snake, TilePosition tilePosition) {
    TilePosition tailTilePosition = snake->tilePosition;

    snake->position.x = tilePosition.col * tileMap->tileWidth;
    snake->position.y = tilePosition.row * tileMap->tileHeight;
    snake->tilePosition.row = tilePosition.row;
    snake->tilePosition.col = tilePosition.col;

    for (int i = 0; i < snake->tailLength; i++) {
        SnakeTail *tail = &snake->tail[i];
        TilePosition tempTilePosition = tail->tilePosition;

        tail->position.x = tailTilePosition.col * tileMap->tileWidth;
        tail->position.y = tailTilePosition.row * tileMap->tileHeight;
        tail->tilePosition.row = tailTilePosition.row;
        tail->tilePosition.col = tailTilePosition.col;

        tailTilePosition = tempTilePosition;
    }

    if (!snake->hasMove) {
        snake->hasMove = true;
    }
}

Item* GameAllocateItem(Game *game, ItemType type) {
    for (int i = 0; i < GAME_MAX_ITEMS; i++) {
        if (game->items[i].type == ITEM_NONE) {
            game->items[i].type = type;
            return &game->items[i];
        }
    }

    return NULL;
}

void GameSpawnItem(Game *game, ItemType type, TilePosition tilePosition) {
    Item *item = GameAllocateItem(game, type);

    if (item == NULL) {
        printf("Fail to spawn item of type %d\n", type);
    } else if (item->type == ITEM_APPLE) {
        item->width = game->tileMap.tileWidth;
        item->height = game->tileMap.tileHeight;
        item->tilePosition = tilePosition;
        item->position.x = tilePosition.col * game->tileMap.tileWidth;
        item->position.y = tilePosition.row * game->tileMap.tileHeight;
        item->spawnTime = GetTime();
        item->spawnElapsedTime = 0;
        item->lifeTime = 5;
        item->scorePoints = 5;
        game->appleSpawnCount++;

        printf("Spawn apple [%d:%d]\n", tilePosition.row, tilePosition.col);
    }
}

Item* GetClosestItem(Game *game, Vector2 position) {
    Item *closestItem = NULL;
    float closestItemDistance = FLT_MAX;

    for (int i = 0; i < GAME_MAX_ITEMS; i++) {
        Item *item = &game->items[i];
        if (item->type == ITEM_APPLE) {
            if (closestItem == NULL) {
                closestItem = item;
            } else {
                float distance = Vector2Distance(item->position, closestItem->position);
                if (distance < closestItemDistance) {
                    closestItemDistance = distance;
                    closestItem = item;
                }
            }
        }
    }

    return closestItem;
}

void GameDespawnItem(Game *game, Item *item) {
    if (item->type == ITEM_APPLE) {
        game->appleSpawnCount--;
        game->appleLastDespawnTime = GetTime();
    }

    item->type = ITEM_NONE;
}

void GameUpdateItems(Game *game) {
    if (game->isPaused) {
        return;
    }

    if (IsKeyPressed(KEY_ENTER)) {
        TilePosition tilePosition;

        if (GameGetRandomEmptyTile(game, &tilePosition)) {
            GameSpawnItem(game, ITEM_APPLE, tilePosition);
        }
    }

    double elapsedTimeSinceLastAppleDespawn = GetTime() - game->appleLastDespawnTime;

    if (game->appleSpawnCount < 1 && elapsedTimeSinceLastAppleDespawn >= game->appleSpawnRate) {
        TilePosition tilePosition;

        if (GameGetRandomEmptyTile(game, &tilePosition)) {
            GameSpawnItem(game, ITEM_APPLE, tilePosition);
        }
    }

    for (int i = 0; i < GAME_MAX_ITEMS; i++) {
        Item *item = &game->items[i];

        if (item->type != ITEM_NONE) {
            item->spawnElapsedTime = GetTime() - item->spawnTime;

            if (item->spawnElapsedTime >= item->lifeTime) {
                GameDespawnItem(game, item);
            }
        }
    }
}

void GameDrawItems(Game *game) {
    for (int i = 0; i < GAME_MAX_ITEMS; i++) {
        Item *item = &game->items[i];

        if (item->type == ITEM_APPLE) {
            DrawRectangle(item->position.x, item->position.y, item->width, item->height, YELLOW);
        }
    }
}

void GameDrawSnake(Snake *snake) {
    float eyeRadius = (snake->headWidth + snake->headHeight) * 0.08;
    float eyePadding = 5;
    Color eyeColor = WHITE;

    Vector2 leftEyeCenter = {
        .x = snake->position.x + eyeRadius + eyePadding,
        .y = snake->headHeight / 2 + snake->position.x
    };

    Vector2 rightEyeCenter = {
        .x = snake->position.x + snake->headWidth - eyeRadius - eyePadding,
        .y = snake->headHeight / 2 + snake->position.x
    };

    if (snake->direction.x > 0) {
        leftEyeCenter.x = snake->position.x + snake->headWidth - eyeRadius - eyePadding;
        leftEyeCenter.y = snake->position.y + eyeRadius + eyePadding;
        rightEyeCenter.x = snake->position.x + snake->headWidth - eyeRadius - eyePadding;
        rightEyeCenter.y = snake->position.y + snake->headHeight - eyeRadius - eyePadding;
    } else if (snake->direction.x < 0) {
        leftEyeCenter.x = snake->position.x + eyeRadius + eyePadding;
        leftEyeCenter.y = snake->position.y + snake->headHeight - eyeRadius - eyePadding;
        rightEyeCenter.x = snake->position.x + eyeRadius + eyePadding;
        rightEyeCenter.y = snake->position.y + eyeRadius + eyePadding;
    } else if (snake->direction.y > 0) {
        leftEyeCenter.x = snake->position.x + snake->headWidth - eyeRadius - eyePadding;
        leftEyeCenter.y = snake->position.y + snake->headHeight - eyeRadius - eyePadding;
        rightEyeCenter.x = snake->position.x + eyeRadius + eyePadding;
        rightEyeCenter.y = snake->position.y + snake->headHeight - eyeRadius - eyePadding;
    } else if (snake->direction.y < 0) {
        leftEyeCenter.x = snake->position.x + eyeRadius + eyePadding;
        leftEyeCenter.y = snake->position.y + eyeRadius + eyePadding;
        rightEyeCenter.x = snake->position.x + snake->headWidth - eyeRadius - eyePadding;
        rightEyeCenter.y = snake->position.y + eyeRadius + eyePadding;
    }

    float eyeDotRadius = eyeRadius * 0.4;
    float eyeDotPadding = 2;
    Color eyeDotColor = BLACK;

    Vector2 leftEyeDotCenter = {
        .x = leftEyeCenter.x + (snake->lookDirection.x * (eyeRadius - eyeDotRadius - eyeDotPadding)),
        .y = leftEyeCenter.y + (snake->lookDirection.y * (eyeRadius - eyeDotRadius - eyeDotPadding))
    };

    Vector2 rightEyeDotCenter = {
        .x = rightEyeCenter.x + (snake->lookDirection.x * (eyeRadius - eyeDotRadius - eyeDotPadding)),
        .y = rightEyeCenter.y + (snake->lookDirection.y * (eyeRadius - eyeDotRadius - eyeDotPadding))
    };

    // Draw tail
    for (int i = 0; i < snake->tailLength; i++) {
        SnakeTail *tail = &snake->tail[i];
        DrawRectangle(tail->position.x, tail->position.y, tail->width, tail->height, ColorBrightness(GREEN, Clamp(i * 0.05, 0.0, 0.5)));
    }

    // Draw head
    DrawRectangle(snake->position.x, snake->position.y, snake->headWidth, snake->headHeight, GREEN);

    // Draw eyes
    DrawCircle(leftEyeCenter.x, leftEyeCenter.y, eyeRadius, eyeColor);
    DrawCircle(rightEyeCenter.x, rightEyeCenter.y, eyeRadius, eyeColor);

    // Draw eyes dots
    DrawCircle(leftEyeDotCenter.x, leftEyeDotCenter.y, eyeDotRadius, eyeDotColor);
    DrawCircle(rightEyeDotCenter.x, rightEyeDotCenter.y, eyeDotRadius, eyeDotColor);
}

void GameDrawTileMap(TileMap *tileMap) {
    for (int col = 0; col < tileMap->cols; col++) {
        for (int row = 0; row < tileMap->rows; row++) {
            Color tileColor = (row + col) % 2 == 0 ? LIGHTGRAY : GRAY;
            DrawRectangle(col * tileMap->tileWidth, row * tileMap->tileHeight, tileMap->tileWidth, tileMap->tileHeight, tileColor);
        }
    }
}

void GameDrawUI(Game *game) {
    char scoreText[32];
    sprintf(scoreText, "Score %d", game->score);
    DrawText(scoreText, 5, 5, 20, BLACK);

    if (game->isPaused) {
        // Overlay
        DrawRectangle(0, 0, game->viewportWidth, game->viewportHeight, ColorAlpha(BLACK, 0.5));

        // Pause Text
        char *text = "Paused";
        int fontSize = 30;
        int textSize = MeasureText(text, fontSize);
        DrawText(text, game->viewportWidth / 2 - textSize / 2, game->viewportHeight / 2 - fontSize / 2, fontSize, WHITE);
    }
}

void GameSnakeHitItem(Game *game, Item *item) {
    if (item->type == ITEM_APPLE) {
        game->score += item->scorePoints;
        GameGrowSnake(&game->snake);
        game->snake.speed += 1;
        GameDespawnItem(game, item);
        PlaySound(game->eatSound);
    }
}

bool GameSnakeHitItself(Snake *snake) {

    for (int i = 0; i < snake->tailLength; i++) {
        if (GameIsTilePositionEqual(snake->tilePosition, snake->tail[i].tilePosition)) {
            return true;
        }
    }

    return false;
}

void GameUpdateSnake(Game *game) {
    if (game->isPaused) {
        return;
    }

    Snake *snake = &game->snake;

    snake->moveTimer.elapsedTime = GetTime() - snake->moveTimer.previousTime;

    if (IsKeyPressed(KEY_SPACE)) {
        if (snake->tailLength < GAME_SNAKE_TAIL_MAX_LENGTH) {
            GameGrowSnake(snake);
        }
    }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        snake->direction.x = -1;
        snake->direction.y = 0;
    }

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        snake->direction.x = 1;
        snake->direction.y = 0;
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        snake->direction.x = 0;
        snake->direction.y = -1;
    }

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        snake->direction.x = 0;
        snake->direction.y = 1;
    }

    if (IsKeyPressed(KEY_KP_ADD)) {
        snake->speed += 1;
    }

    if (IsKeyPressed(KEY_KP_SUBTRACT)) {
        if (snake->speed > 1) {
            snake->speed -= 1;
        }
    }

    snake->lookDirection.x = snake->direction.x;
    snake->lookDirection.y = snake->direction.y;

    Item * closestItem = GetClosestItem(game, snake->position);

    if (closestItem != NULL) {
        Vector2 direction = Vector2Normalize(Vector2Subtract(closestItem->position, snake->position));
        snake->lookDirection.x = direction.x;
        snake->lookDirection.y = direction.y;
    }

    if (snake->direction.x != 0 || snake->direction.y != 0) {
        double timeToNextMove = 1 / snake->speed;

        if (snake->moveTimer.elapsedTime >= timeToNextMove) {
            TilePosition targetTilePosition;

            GameGetTilePositionFromDirection(snake->direction, snake->tilePosition, &targetTilePosition);

            if (targetTilePosition.row < 0) {
                targetTilePosition.row = game->tileMap.rows - 1;
            } else if (targetTilePosition.row >= game->tileMap.rows) {
                targetTilePosition.row = 0;
            }

            if (targetTilePosition.col < 0) {
                targetTilePosition.col = game->tileMap.cols - 1;
            } else if (targetTilePosition.col >= game->tileMap.cols) {
                targetTilePosition.col = 0;
            }

            // if (GameIsTileValid(&game->tileMap, targetTilePosition)) {
            //     int targetTileValue = GameGetTileValue(&game->tileMap, targetTilePosition);

            //     if (targetTileValue != TILE_WALL) {
            //         GameMoveSnake(&game->tileMap, snake, targetTilePosition);
            //     }
            // }

            GameMoveSnake(&game->tileMap, snake, targetTilePosition);

            snake->moveTimer.previousTime = GetTime();
        }
    }

    Item *hitItem = GameCheckSnakeHitsItem(game, snake);

    if (hitItem != NULL) {
        GameSnakeHitItem(game, hitItem);
    }

    if (snake->hasMove && GameSnakeHitItself(snake)) {
        printf("Snakehit itself\n");
    }
}

void GameUpdate(Game *game) {
    if (IsKeyPressed(KEY_SPACE)) {
        game->isPaused = !game->isPaused;
    }

    GameUpdateItems(game);
    GameUpdateSnake(game);
}

void GameDraw(Game *game) {
    ClearBackground(BLACK);

    GameDrawTileMap(&game->tileMap);
    GameDrawItems(game);
    GameDrawSnake(&game->snake);

    GameDrawUI(game);
}

TileMap GameCreateTileMap(int rows, int cols, float tileWidth, float tileHeight) {
    TileValue *tiles = (TileValue*) malloc(sizeof(TileValue) * rows * cols);

    for (int i = 0; i < rows * cols; i++) {
        tiles[i] = TILE_EMPTY;
    }

    TileMap tileMap = {
        .rows = rows,
        .cols = cols,
        .tileWidth = tileWidth,
        .tileHeight = tileHeight,
        .tiles = tiles,
    };

    return tileMap;
}

void GameFreeTileMap(TileMap *tileMap) {
    free(tileMap->tiles);
}

void GameInit(Game *game, int width, int height) {
    game->viewportWidth = width;
    game->viewportHeight = height;
    game->score = 0;

    int rows = 20;
    int cols = 20;

    TileValue *tiles = (TileValue*) malloc(sizeof(TileValue) * rows * cols);

    for (int i = 0; i < rows * cols; i++) {
        tiles[i] = TILE_EMPTY;
    }

    TilePosition initTilePosition = {.row = 1, .col = 1};

    game->isPaused = false;
    game->tileMap.rows = rows;
    game->tileMap.cols = cols;
    game->tileMap.tileWidth = width / cols;
    game->tileMap.tileHeight = height / rows;
    game->tileMap.tiles = tiles;
    game->appleSpawnRate = 2;
    game->eatSound = LoadSound("assets/eat.ogg");

    game->snake.headWidth = game->tileMap.tileWidth;
    game->snake.headHeight = game->tileMap.tileHeight;
    game->snake.direction.x = 0;
    game->snake.direction.y = 0;
    game->snake.tailLength = 1;
    game->snake.speed = 5; // Tiles per second
    game->snake.moveTimer.elapsedTime = 0;
    game->snake.moveTimer.previousTime = 0;
    game->snake.position.x = initTilePosition.col * game->tileMap.tileWidth;
    game->snake.position.y = initTilePosition.row * game->tileMap.tileHeight;
    game->snake.tilePosition.row = initTilePosition.row;
    game->snake.tilePosition.col = initTilePosition.col;
    game->snake.hasMove = false;

    for (int i = 0; i < GAME_SNAKE_TAIL_MAX_LENGTH; i++) {
        game->snake.tail[i].width = game->tileMap.tileWidth;
        game->snake.tail[i].height = game->tileMap.tileHeight;
        game->snake.tail[i].position.x = game->snake.position.x;
        game->snake.tail[i].position.y = game->snake.position.y;
        game->snake.tail[i].tilePosition.row = initTilePosition.row;
        game->snake.tail[i].tilePosition.col = initTilePosition.col;
    }
}

void GameExit(Game *game) {
    free(game->tileMap.tiles);
}

int main(void) {
    int windowWidth = 800;
    int windowHeight = 800;

    InitWindow(windowWidth, windowHeight, "Snake Game");
    InitAudioDevice();

    Game game;
    GameInit(&game, windowWidth, windowHeight);

    while (!WindowShouldClose()) {
        GameUpdate(&game);

        BeginDrawing();

        GameDraw(&game);

        EndDrawing();
    }

    GameExit(&game);

    CloseWindow();
    CloseAudioDevice();

    return 0;
}
