import os
import requests
import dash
from dash import html, dcc, dash_table
from dash.dependencies import Input, Output

BACKEND_URL = os.getenv("BACKEND_URL", "http://localhost:8000")

app = dash.Dash(__name__, title="Firefighter Robot Fleet")

LEVEL_COLORS = {
    "HIGH":     "#ff4444",
    "MEDIUM":   "#ff9900",
    "LOW":      "#ffdd00",
    "SURVIVOR": "#00ccff",
}

app.layout = html.Div(style={"fontFamily": "monospace", "padding": "20px"}, children=[
    html.H2("Firefighter Robot Fleet — Command Dashboard"),
    dcc.Interval(id="refresh", interval=2000, n_intervals=0),  # poll every 2s

    html.Div(id="summary", style={"marginBottom": "20px"}),

    dash_table.DataTable(
        id="alert-table",
        columns=[
            {"name": "Robot",     "id": "robot_id"},
            {"name": "Type",      "id": "type"},
            {"name": "Level",     "id": "level"},
            {"name": "Data",      "id": "data"},
            {"name": "Timestamp", "id": "timestamp"},
        ],
        style_data_conditional=[
            {"if": {"filter_query": '{level} = "HIGH"'},     "backgroundColor": "#ff4444", "color": "white"},
            {"if": {"filter_query": '{level} = "MEDIUM"'},   "backgroundColor": "#ff9900", "color": "white"},
            {"if": {"filter_query": '{level} = "SURVIVOR"'}, "backgroundColor": "#00ccff", "color": "black"},
        ],
        style_table={"overflowX": "auto"},
        page_size=20,
    ),
])


@app.callback(
    Output("alert-table", "data"),
    Output("summary", "children"),
    Input("refresh", "n_intervals"),
)
def update_table(n):
    try:
        resp = requests.get(f"{BACKEND_URL}/alerts?limit=100", timeout=2)
        alerts = resp.json()
    except Exception:
        return [], html.Span("Backend unreachable", style={"color": "red"})

    rows = [
        {
            "robot_id":  a["robot_id"],
            "type":      a["type"],
            "level":     a["level"],
            "data":      str(a["data"]),
            "timestamp": f"{a['timestamp']:.1f}",
        }
        for a in alerts
    ]

    survivors = sum(1 for a in alerts if a["level"] == "SURVIVOR")
    high      = sum(1 for a in alerts if a["level"] == "HIGH")

    summary = html.Div([
        html.Span(f"Total alerts: {len(alerts)}  |  ", style={"marginRight": "10px"}),
        html.Span(f"🔴 HIGH: {high}  |  ", style={"color": "#ff4444", "marginRight": "10px"}),
        html.Span(f"🔵 Survivors detected: {survivors}", style={"color": "#00ccff"}),
    ])

    return rows, summary


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8050, debug=False)
