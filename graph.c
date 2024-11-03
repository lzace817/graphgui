#include "raylib.h"
#include "raymath.h"
#include <stddef.h>
// #include <stdio.h>

#define ARRAY_LENGTH(array) sizeof(array)/sizeof(array[0])

#define SCREEN_WIDTH 600
#define SCREEN_HEGHT 400
#define NODE_RADIUS 40
#define NODE_BORDER 4
#define BACKGROUND_COLOR CLITERAL(Color){20,20,20,255}
#define HOVER_COLOR CLITERAL(Color){40,40,40,255}
#define HOVER_MARGIN 10
#define CONTROL_RADIUS 6
#define MIN_CONTROL_DISTANCE 60
#define ARROW_HALF_BASE 8
#define ARROW_LEN 20


void draw_node(Vector2 pos, bool hovering)
{
    if (hovering) {
        Rectangle r1 = {pos.x - NODE_RADIUS - HOVER_MARGIN,
                pos.y - NODE_RADIUS - HOVER_MARGIN,
                2*(NODE_RADIUS + HOVER_MARGIN),
                2*(NODE_RADIUS + HOVER_MARGIN) };
        DrawRectangleRounded(r1, 0.3f, 5, HOVER_COLOR);
    }
    DrawCircle(pos.x, pos.y, NODE_RADIUS + NODE_BORDER, WHITE);
    DrawCircle(pos.x, pos.y, NODE_RADIUS, (hovering)?HOVER_COLOR:BACKGROUND_COLOR);
}

typedef struct Edge {
    int from;
    int to;
    Vector2 ctrl[2];
} Edge;

// rotate a vector by a right angle in the counter-clockwise direction
Vector2 Vector2CounterRight(Vector2 v)
{
    return (Vector2){-v.y, v.x};
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEGHT, "graph primitives");
    Vector2 nodes[] = {
        {SCREEN_WIDTH/3, SCREEN_HEGHT/2},
        {2*SCREEN_WIDTH/3, SCREEN_HEGHT/2},
        {SCREEN_WIDTH/2, SCREEN_HEGHT*0.7f},
    };

    Edge edges[] = {
        {
            0, 1, {
                { 100, -5},
                {0, -120}
            }
        },
        {
            0, 2, {
                { 0, 80},
                {-60, 0}
            }
        },
    };

    bool draw_control = true;
    int focused_node = -1;
    int selected_node = -1;
    int focused_control = -1;
    int selected_control = -1;
    Vector2 selected_offset = {0};
    Vector2 *attached_node = NULL;
    // bool hovering = true;

    SetTargetFPS(30);

    while(!WindowShouldClose()) {
        if(IsKeyPressed(KEY_C)) draw_control = !draw_control;

        for(size_t i = 0; i < ARRAY_LENGTH(nodes); i++){
            if(CheckCollisionPointCircle(GetMousePosition(), nodes[i], NODE_RADIUS)){
                focused_node = i;
                if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    selected_node = i;
                    selected_offset = Vector2Subtract(nodes[i], GetMousePosition());
                }
                break;
            } else focused_node = -1;
        }

        if (draw_control) {
            focused_control = -1;
            for(size_t i = 0; i < ARRAY_LENGTH(edges); i++) {
                Edge e = edges[i];
                if(CheckCollisionPointCircle(GetMousePosition(),
                        Vector2Add(nodes[e.from], e.ctrl[0]), CONTROL_RADIUS))
                {
                    focused_control = 2*i;
                    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        selected_control = 2*i;
                        attached_node = &nodes[e.from];
                    }
                    break;
                }

                if(CheckCollisionPointCircle(GetMousePosition(),
                        Vector2Add(nodes[e.to], e.ctrl[1]), CONTROL_RADIUS))
                {
                    focused_control = 2*i+1;
                    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        selected_control = 2*i + 1;
                        attached_node = &nodes[e.to];
                    }
                    break;
                }

            }

            if(selected_control >= 0) {
                Vector2 new_ctrl_pos = Vector2Subtract(GetMousePosition(),
                        *attached_node);
                float d = Vector2Length(new_ctrl_pos);
                if (d < 0.1f) {
                    new_ctrl_pos = (Vector2){MIN_CONTROL_DISTANCE,0};
                }
                else if (d < MIN_CONTROL_DISTANCE) new_ctrl_pos = Vector2Scale(
                        new_ctrl_pos, MIN_CONTROL_DISTANCE/d);
                int edge_idx = selected_control >> 1;
                edges[edge_idx].ctrl[selected_control & 1] = new_ctrl_pos;

            }

        }

        if(selected_node >= 0) {
            nodes[selected_node] = Vector2Add(GetMousePosition(), selected_offset);
        }



        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)){
            selected_node = -1;
            selected_control = -1;
        }

        // ########################## DRAWING #########################################
        BeginDrawing();
            ClearBackground(BACKGROUND_COLOR);

            for(size_t i = 0; i < ARRAY_LENGTH(nodes); i++){
                draw_node(nodes[i], focused_node == i);
            }

            for(size_t i = 0; i < ARRAY_LENGTH(edges); i++) {
                Edge edge = edges[i];
                Vector2 n1 = nodes[edge.from];
                Vector2 n2 = nodes[edge.to];
                Vector2 bs = Vector2Add(n1, Vector2Scale(Vector2Normalize(edge.ctrl[0]), NODE_RADIUS));
                Vector2 normal_end = Vector2Normalize(edge.ctrl[1]);
                Vector2 be = Vector2Add(n2, Vector2Scale(normal_end, NODE_RADIUS + ARROW_LEN));
                Vector2 c1a = Vector2Add(n1, edge.ctrl[0]);
                Vector2 c2a = Vector2Add(n2, edge.ctrl[1]);

                DrawSplineSegmentBezierCubic(bs, c1a, c2a, be, 4.0f, BLUE);

                Vector2 t1 = Vector2Scale(Vector2CounterRight(normal_end), ARROW_HALF_BASE);
                Vector2 tip = Vector2Add(n2, Vector2Scale(normal_end, NODE_RADIUS));
                Vector2 b1 = Vector2Subtract(be, t1);
                Vector2 b2 = Vector2Add     (be, t1);

                // Draw a color-filled triangle (vertex in counter-clockwise order!)
                DrawTriangle(tip, b2, b1, BLUE);


                if (draw_control) {
                    DrawLineV(bs, c1a, DARKGRAY);
                    DrawLineV(be, c2a, DARKGRAY);

                    DrawCircleV(c1a, 6, (2*i == focused_control)?GREEN:GOLD);
                    DrawCircleV(c2a, 6, ((2*i +1) == focused_control)?GREEN:GOLD);
                }
            }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}