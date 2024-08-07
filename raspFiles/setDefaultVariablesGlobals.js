// Nodo Function para setear las variables globales `placar` y `parameters` con valores por default

// Valores predeterminados para `parameters`
let defaultParameters = {
    "tournamentID": null,
    "tournamentStatus": "Pending",
    "matchID": "matchID",
    "matchStatus": "inProgress",
    "gameMode": "A",
    "players": {
        "teamA": {
            "player1": "name1",
            "player2": "name2"
        },
        "teamB": {
            "player3": "name3",
            "player4": "name4"
        }
    },
    "adData": "dataAdvertising",
    "environment": {
        "temperature": "23",
        "humidity": "29%"
    }
};

// Valores predeterminados para `placar`
let defaultPlacar = {
    match : {
            matchID: "defaultMatchID",
            startTime: msg.timestamp,
            status: "inProgress",
            scoring: {
                currentSet: 0,
                currentGame:[0,0],
                formattedGame:[0,0],
                setsWins: [0,0],
                sets:[[0,0], [0,0],[0,0]],
                inTiebreak:false
            },
        },
        params : { ...defaultParameters },
        history: {
            actions: [],
            scores: []
        },
        command:null
}

// Setear `parameters` y `placar` globales
flow.set('parameters', defaultParameters);
flow.set('placar', defaultPlacar);

// Mensaje de confirmación
msg.payload = {
    message: "Variables globales `placar` y `parameters` seteadas correctamente con valores por default.",
    placar: defaultPlacar,
    parameters: defaultParameters
};

return msg;
