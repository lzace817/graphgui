typedef struct NodePropWnd {
    char NameTextBoxText[128];
    bool editable;
} NodePropWnd;

int GuiNodeProperty(NodePropWnd *wnd, Vector2 position)
{
    int ret = GuiWindowBox((Rectangle){ position.x, position.y, 152, 104 }, "Node properties");
    int NodeWndActive = !ret;
    // TraceLog(LOG_DEBUG, "window box ret = %d", ret);
    GuiLabel((Rectangle){ position.x + 8, position.y + 32, 32, 24 }, "Name");
    if (GuiTextBox((Rectangle){ position.x + 48, position.y + 32, 96, 24 }, wnd->NameTextBoxText, 128, wnd->editable))
        wnd->editable = !wnd->editable;
    if (GuiButton((Rectangle){ position.x + 8, position.y + 72, 64, 24 }, "Cancel"))
        NodeWndActive = false;
    if (GuiButton((Rectangle){ position.x + 80, position.y + 72, 64, 24 }, "Ok")) {
        // set the properties
        TraceLog(LOG_DEBUG, "set name to \"%s\"", wnd->NameTextBoxText);
        return false;
    }
    return NodeWndActive;
}
