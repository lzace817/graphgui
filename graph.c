#include "raylib.h"
#include "raymath.h"
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "raygui.h"
#include "style_dark.h"
#include "subwindows.c"

#define COMMONS_IMPLEMENTATION
#include "commons.h"

#define SCREEN_WIDTH 600
#define SCREEN_HEGHT 400
#define ORIGIN_LINE_LEN 40
#define ORIGIN_CIRCLE_RADIUS 15
#define NODE_RADIUS 40
#define NODE_BORDER 4
#define BACKGROUND_COLOR CLITERAL(Color){20,20,20,255}
#define HOVER_COLOR CLITERAL(Color){40,40,40,255}
#define ORIGIN_COLOR CLITERAL(Color){255,255,255,255}
#define HOVER_MARGIN 10
#define CONTROL_RADIUS 6
#define MIN_CONTROL_DISTANCE 60
#define ARROW_HALF_BASE 8
#define ARROW_LEN 20
#define UI_FONT_SIZE 25.0f
#define BORDER_COLOR CLITERAL(Color){40,40,40,255}

typedef struct Edge {
    int from;
    int to;
    Vector2 ctrl[2];
    Vector2 loffset;
    char label[16]; // TODO: make it resizeable
} Edge;

typedef struct GraphCtx {
    float  zoom_coef;
    Font font;
    int focused;
    int active;
    int id_type;
    bool show_control_pts;
} GraphCtx;

// EdgeGeo
// start, c1, c2, end, tip, b1, b2, lpos, lsize
typedef struct EdgeGeo {
    Vector2 points[9];
} EdgeGeo;

// rotate a vector by a right angle in the counter-clockwise direction
Vector2 Vector2CounterRight(Vector2 v)
{
    return (Vector2){-v.y, v.x};
}

enum GraphColors {
    GC_NODE,
    GC_EDGE,
    GC_EDGE_ACTIVE,
    GC_CONTROL,
    GC_CONTROL_LINE,
    GC_CONTROL_SELECTED,
    GC_LABEL,
    GC_LABEL_BACKGROUND_HOVER,

    GC_NUM_ITEMS
};

enum EdgeItems{
    EI_BS,
    EI_C1A,
    EI_C2A,
    EI_BE,
    EI_TIP,
    EI_B1,
    EI_B2,
    EI_LPOS,
    EI_LSIZE,
};

enum IdType {
    IT_NODE,
    IT_EDGE,
    IT_LABEL,
    IT_CRTL_PT1,
    IT_CRTL_PT2,
    IT_DRAWING,
    IT_WINDOW,

    IT_NONE = -1,
};

enum ToolId {
    TI_CURSOR,
    TI_ADD_NODE,
    TI_REM_NODE,
    TI_DEBUG,
};

static Color global_graph_colors[] = {
    [GC_NODE] = WHITE,
    [GC_EDGE] = BLUE,
    [GC_EDGE_ACTIVE] = RED,
    [GC_CONTROL] = GOLD,
    [GC_CONTROL_LINE] = LIGHTGRAY,
    [GC_CONTROL_SELECTED] = GREEN,
    [GC_LABEL] = LIGHTGRAY,
    [GC_LABEL_BACKGROUND_HOVER] = {255,255,255, 51},
};

static_assert(ARRAYSIZE(global_graph_colors) == GC_NUM_ITEMS);
#define internal static

#define focus(type, id)           \
    do { if (ctx.active < 0) {    \
        ctx.focused = id;         \
        ctx.id_type = type;       \
    } } while(0)

internal Color graph_color(enum GraphColors c)
{
    return global_graph_colors[c];
}

inline internal Rectangle label_rect(EdgeGeo *geo)
{
    // bs, c1, c2, be, tip, b1a, b2a, lpos, lsize
    Vector2 lpos = geo->points[EI_LPOS];
    Vector2 lsize = geo->points[EI_LSIZE];
    Rectangle result = {lpos.x, lpos.y, lsize.x, lsize.y};
    return result;
}

void draw_node(Vector2 pos, bool hovering)
{
    if (hovering) {
        Rectangle r1 = {pos.x - NODE_RADIUS - HOVER_MARGIN,
                pos.y - NODE_RADIUS - HOVER_MARGIN,
                2*(NODE_RADIUS + HOVER_MARGIN),
                2*(NODE_RADIUS + HOVER_MARGIN) };
        DrawRectangleRounded(r1, 0.3f, 5, HOVER_COLOR);
    }
    DrawRing(pos, NODE_RADIUS, NODE_RADIUS + NODE_BORDER, 0.0f, 360.0f, 0, graph_color(GC_NODE)); // Draw ring
}

void draw_edge(EdgeGeo *geo, int id, const char *label, GraphCtx *ctx);
void compute_edge_geo(EdgeGeo *geo, Vector2 n1, Vector2 n2, Vector2 c1, Vector2 c2,
        Vector2 loffset, const char *label, GraphCtx *ctx);

int main(void)
{
    // enable debug tracing
    SetTraceLogLevel(LOG_DEBUG);

    InitWindow(SCREEN_WIDTH, SCREEN_HEGHT, "graphgui");
    SetExitKey(KEY_Q);
    GraphCtx ctx;
    ctx.font = LoadFontEx("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", UI_FONT_SIZE, 0, 250);
    GuiLoadStyleDark();

    Camera2D camera = {0};
    camera.zoom = 1.0f;
    float scrollSpeed = 0.2f;

    Vector2 *nodes = NULL;
    {
        Vector2 v1 = {SCREEN_WIDTH/3, SCREEN_HEGHT/2};
        Vector2 v2 = {2*SCREEN_WIDTH/3, SCREEN_HEGHT/2};
        Vector2 v3 = {SCREEN_WIDTH/2, SCREEN_HEGHT*0.7f};
        da_append(nodes, v1);
        da_append(nodes, v2);
        da_append(nodes, v3);
    };

    Edge *edges = NULL;
    {
        da_append(edges, ((Edge){
            0, 1, {
                { 100, -5},
                {0, -120}
            },
            {0, 0},
            "hello"
        }));

        da_append(edges, ((Edge){
            0, 2, {
                { 0, 80},
                {-75, 0}
            },
            {0, 0},
            "world"
        }));

        da_append(edges, ((Edge){
            1, 2, {
                { 0, 100},
                {70, 60}
            },
            {0, 0},
            "!"
        }));

        da_append(edges, ((Edge){
            1, 1, {
                { 90, -50},
                {90, 50}
            },
            {0, 0},
            "repeat"
        }));

    }

    EdgeGeo *edge_geo = NULL;
    for(size_t i = 0; i < da_size(edges); i++)
        da_append(edge_geo, (EdgeGeo){0});

    ctx.show_control_pts = false;
    ctx.focused = -1;
    ctx.active = -1;
    ctx.id_type = IT_NONE;
    Vector2 selected_offset = {0};
    Vector2 *attached_node = NULL;
    int active_tool = TI_CURSOR;
    Vector2 preview_node;
    Rectangle drawing_area = {96, 0, SCREEN_WIDTH - 96, SCREEN_HEGHT};
    NodePropWnd nodewnd = {0};
    // bool hovering = true;


    SetTargetFPS(30);

    while(!WindowShouldClose()) {
        // camera.zoom += (int)(GetMouseWheelMove()*scrollSpeed);
        {
            float new_zoom = camera.zoom + ((float)GetMouseWheelMove()*0.05f);
            if(new_zoom < 0.05f) new_zoom = 0.05f;
            float s = (new_zoom - camera.zoom)/(new_zoom*camera.zoom);
            camera.target = Vector2Add(camera.target, Vector2Scale(GetMousePosition(), s));
            camera.zoom = new_zoom;
        }
        // pan control
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            Vector2 delta = GetMouseDelta();
            delta = Vector2Scale(delta, -1.0f/camera.zoom);
            camera.target = Vector2Add(camera.target, delta);
        }

        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);

        if (IsKeyPressed(KEY_D)) {
            // TraceLog(LOG_DEBUG, "item active/focused: %d, %d, %d", ctx.id_type, ctx.active, ctx.focused);
            TraceLog(LOG_DEBUG, "camera zoom %f", camera.zoom);
            // TraceLog(LOG_DEBUG, "wheel %f", GetMouseWheelMove());
        }

        if (active_tool == TI_CURSOR) {
            // if (IsKeyPressed(KEY_C))
            //     ctx.show_control_pts = !ctx.show_control_pts;

            // move nodes
            if (ctx.id_type == IT_NODE && ctx.active >= 0) {
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    ctx.focused = -1;
                    ctx.active  = -1;
                    ctx.id_type = -1;
                } else
                nodes[ctx.active] = Vector2Add(mouseWorldPos, selected_offset);
            }

            // move edges
            if (ctx.id_type == IT_LABEL && ctx.active >= 0) {
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    ctx.focused = -1;
                    ctx.active  = -1;
                    ctx.id_type = -1;
                } else

                edges[ctx.active].loffset = Vector2Add(mouseWorldPos, selected_offset);
            }

            if (ctx.active < 0) {
                ctx.focused = -1;
                ctx.id_type = -1;
                ctx.active = -1;
            }

            // all nodes
            for (size_t i = 0; i < da_size(nodes); i++) {
                if (CheckCollisionPointCircle(mouseWorldPos, nodes[i], NODE_RADIUS)) {
                    focus(IT_NODE, i);
                }
                if (ctx.id_type == IT_NODE && ctx.focused == (int)i) {
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        ctx.active      = i;
                        selected_offset = Vector2Subtract(nodes[i], mouseWorldPos);
                    }
                    break;
                }
            }

            // all edges
            if (ctx.id_type == IT_CRTL_PT1 || ctx.id_type == IT_CRTL_PT2) {
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    ctx.id_type = -1;
                    ctx.active = -1;
                    ctx.focused = -1;
                }
            }
            float control_radius_world = CONTROL_RADIUS / camera.zoom;
            for (size_t i = 0; i < da_size(edges); i++) {
                Edge e = edges[i];
                compute_edge_geo(edge_geo + i, nodes[e.from], nodes[e.to], e.ctrl[0], e.ctrl[1],
                                 e.loffset, e.label, &ctx);

                if (ctx.show_control_pts) {
                    // control point 1
                    if (CheckCollisionPointCircle(mouseWorldPos, edge_geo[i].points[EI_C1A],
                                                  control_radius_world )) {
                        focus(IT_CRTL_PT1, i);
                        // TraceLog(LOG_DEBUG, "control 1");
                    }
                    if (ctx.id_type == IT_CRTL_PT1 && ctx.focused == (int)i) {
                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            ctx.active    = i;
                            ctx.id_type   = IT_CRTL_PT1;
                            attached_node = &nodes[e.from];
                        }
                        break;
                    }


                    // control point 2

                    if (CheckCollisionPointCircle(mouseWorldPos, edge_geo[i].points[EI_C2A],
                                                  control_radius_world)) {
                        focus(IT_CRTL_PT2, i);
                        // TraceLog(LOG_DEBUG, "control 2");
                    }
                    if (ctx.id_type == IT_CRTL_PT2 && ctx.focused == (int)i) {
                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            ctx.active    = i;
                            ctx.id_type   = IT_CRTL_PT2;
                            attached_node = &nodes[e.to];
                        }
                        break;
                    }
                }

                // label
                Rectangle rec = label_rect(edge_geo + i);
                if (CheckCollisionPointRec(mouseWorldPos, rec)) {
                    focus(IT_LABEL, i);
                }
                if (ctx.id_type == IT_LABEL && ctx.focused == (int)i) {
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        ctx.active      = i;
                        ctx.id_type     = IT_LABEL;
                        selected_offset = Vector2Subtract(e.loffset, mouseWorldPos);
                    }
                    break;
                }
            }

            // move control points
            if ((ctx.id_type == IT_CRTL_PT1 || ctx.id_type == IT_CRTL_PT2) && ctx.active >= 0) {
                Vector2 new_ctrl_pos = Vector2Subtract(mouseWorldPos, *attached_node);
                float d              = Vector2Length(new_ctrl_pos);
                if (d < 0.1f) {
                    new_ctrl_pos = (Vector2){ MIN_CONTROL_DISTANCE, 0 };
                } else if (d < MIN_CONTROL_DISTANCE)
                    new_ctrl_pos = Vector2Scale(new_ctrl_pos, MIN_CONTROL_DISTANCE / d);
                edges[ctx.active].ctrl[ctx.id_type - IT_CRTL_PT1] = new_ctrl_pos;
            }
        } else
        if (active_tool == TI_ADD_NODE) {
            if (ctx.active < 0) {
                ctx.focused = -1;
                ctx.id_type = -1;
                ctx.active = -1;
            }
            preview_node = mouseWorldPos;
            if (ctx.id_type == IT_DRAWING && ctx.active == 0) {
                if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    if (CheckCollisionPointRec(GetMousePosition(), drawing_area)){
                        // da_append(nodes, mouseWorldPos);
                        TraceLog(LOG_DEBUG, "node placed at: %f, %f", preview_node.x, preview_node.y);
                    }
                    ctx.focused = -1;
                    ctx.id_type = -1;
                    ctx.active = -1;
                }
            }
            if(CheckCollisionPointRec(GetMousePosition(), drawing_area))
            {
                focus(IT_DRAWING, 0);
            }
            if (ctx.id_type == IT_DRAWING && ctx.focused == 0) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    ctx.active = 0;
                    ctx.id_type = IT_DRAWING;
                }
            }
        }




        // ########################## DRAWING #########################################
        ctx.zoom_coef = 1.0f/camera.zoom;
        BeginDrawing();

            ClearBackground(BACKGROUND_COLOR);

            DrawLine(drawing_area.x, 0, drawing_area.x, drawing_area.height, BORDER_COLOR);

            BeginScissorMode(drawing_area.x, drawing_area.y, drawing_area.width, drawing_area.height);
            // draw origin
            Vector2 origin = GetWorldToScreen2D((Vector2){0}, camera);
            DrawLine(origin.x - ORIGIN_LINE_LEN/2, origin.y, origin.x + ORIGIN_LINE_LEN/2, origin.y, ORIGIN_COLOR);
            DrawLine(origin.x , origin.y - ORIGIN_LINE_LEN/2, origin.x, origin.y + ORIGIN_LINE_LEN/2, ORIGIN_COLOR);
            DrawCircleLinesV(origin, ORIGIN_CIRCLE_RADIUS, ORIGIN_COLOR);
            BeginMode2D(camera);
                for(size_t i = 0; i < da_size(nodes); i++){
                    draw_node(nodes[i], ctx.id_type == IT_NODE && ctx.focused == (int)i);
                }

                for(size_t i = 0; i < da_size(edges); i++) {
                    Edge edge = edges[i];
                    draw_edge(edge_geo + i, i, edge.label, &ctx);
                }
                // DrawTextEx(ctx.font, "press C to toggle control points", (Vector2){10,10},
                //         UI_FONT_SIZE, 2.0f, WHITE);
                if (ctx.id_type == IT_DRAWING) {
                    draw_node(preview_node, false);
                }
            EndMode2D();
            EndScissorMode();

            GuiToggle((Rectangle){10, 10, 80,30}, "Ctrl pts", &ctx.show_control_pts);
            GuiToggleGroup((Rectangle){ 10, 50, 30, 30 }, "#21#\n#23#\n#28#\n#128#", &active_tool);
            if (GuiButton((Rectangle){ 10, 180, 64, 30 }, "window")) {
                memset(&nodewnd, 0, sizeof(nodewnd));
                // nodewnd.active = true;
                ctx.id_type = IT_WINDOW;
                ctx.active = 0;
            }
            if (ctx.id_type == IT_WINDOW && ctx.active == 0) {
                int active = GuiNodeProperty(&nodewnd, (Vector2){100,100});
                if (! active) {
                    ctx.id_type = -1;
                    ctx.active = -1;
                }
            }

        EndDrawing();
    }

    da_free(nodes);
    da_free(edges);
    da_free(edge_geo);

    UnloadFont(ctx.font);
    CloseWindow();
    return 0;
}

void draw_edge(EdgeGeo *geo, int id, const char *label, GraphCtx *ctx)
{
    // DrawSplineSegmentBezierCubic(bs, c1a, c2a, be, 4.0f, graph_color(GC_EDGE));
    Color edge_color;
    if(ctx->id_type == IT_LABEL && ctx->active == id)
        edge_color = graph_color(GC_EDGE_ACTIVE);
    else
        edge_color = graph_color(GC_EDGE);

    DrawSplineBezierCubic(geo->points, 4, 4.0f, edge_color);

    Vector2 lpos = geo->points[EI_LPOS];
    Rectangle rec = label_rect(geo);
    if (ctx->id_type == IT_LABEL && ctx->focused == id ) DrawRectangleRec(rec,
            graph_color(GC_LABEL_BACKGROUND_HOVER));
    DrawTextEx(ctx->font, label, lpos, UI_FONT_SIZE, 2.0f, graph_color(GC_LABEL));

    // DrawTriangle(tip, b2, b1, graph_color(GC_EDGE));
    DrawTriangleFan(geo->points + EI_TIP, 3, edge_color);

    if (ctx->show_control_pts) {
        float ctrl_radius = CONTROL_RADIUS * ctx->zoom_coef;
        Vector2 bs = geo->points[EI_BS];
        Vector2 be = geo->points[EI_BE];
        Vector2 c1a = geo->points[EI_C1A];
        Vector2 c2a = geo->points[EI_C2A];
        DrawLineV(bs, c1a, graph_color(GC_CONTROL_LINE));
        DrawLineV(be, c2a, graph_color(GC_CONTROL_LINE));

        DrawCircleV(c1a, ctrl_radius, (ctx->id_type == IT_CRTL_PT1 && id == ctx->focused)?
                graph_color(GC_CONTROL_SELECTED):graph_color(GC_CONTROL));
        DrawCircleV(c2a, ctrl_radius, (ctx->id_type == IT_CRTL_PT2 && id == ctx->focused)?
                graph_color(GC_CONTROL_SELECTED):graph_color(GC_CONTROL));
    }
}

void compute_edge_geo(EdgeGeo *geo, Vector2 n1, Vector2 n2, Vector2 c1, Vector2 c2, Vector2 loffset,
        const char *label, GraphCtx *ctx)
{
    Vector2 bs = Vector2Add(n1, Vector2Scale(Vector2Normalize(c1), NODE_RADIUS));
    Vector2 normal_end = Vector2Normalize(c2);
    Vector2 be = Vector2Add(n2, Vector2Scale(normal_end, NODE_RADIUS + ARROW_LEN));
    Vector2 c1a = Vector2Add(n1, c1);
    Vector2 c2a = Vector2Add(n2, c2);

    geo->points[EI_BS] = bs;
    geo->points[EI_C1A] = c1a;
    geo->points[EI_C2A] = c2a;
    geo->points[EI_BE] = be;

    Vector2 lpos = GetSplinePointBezierCubic(bs, c1a, c2a, be, 0.5f);
    Vector2 lsize = MeasureTextEx(ctx->font, label, UI_FONT_SIZE, 2.0f);
    lpos = Vector2Add(lpos, loffset);

    geo->points[EI_LPOS]  = lpos;
    geo->points[EI_LSIZE] = lsize;

    Vector2 t1 = Vector2Scale(Vector2CounterRight(normal_end), ARROW_HALF_BASE);
    Vector2 tip = Vector2Add(n2, Vector2Scale(normal_end, NODE_RADIUS));
    Vector2 b2 = Vector2Subtract(be, t1);
    Vector2 b1 = Vector2Add     (be, t1);

    geo->points[EI_TIP] = tip;
    geo->points[EI_B1]  = b1;
    geo->points[EI_B2]  = b2;
}