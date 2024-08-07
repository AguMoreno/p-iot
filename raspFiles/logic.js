    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // Obtener parámetros globales
    let parameters = flow.get('parameters'); 

    // Obtener el objeto 'placar' del contexto de flujo
    let placar = flow.get('placar') || newPlacar(parameters);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // Evaluar la acción proveniente del nodo inject
    switch (msg.payload.action) {
        case "A":
            console.log("/////********recieved Action A");
            if (placar.match.status === "inProgress") {
                updateScore(0);
            }
            break;
        case "B":
            console.log("/////********recieved Action B");
            if (placar.match.status === "inProgress") {
                updateScore(1);
            }
            break;
        case "Undo":
            console.log("/////********recieved Action Undo Score");
            if (placar.match.status === "inProgress") {
                undoScore();
            }
            break;
        case "Reset":
            console.log("/////********recieved Action Reset Score");
            if (placar.match.status === "inProgress") {
                resetScore();
            }
            break;
        case "Reboot":
            console.log("/////********recieved Action Reboot Placar");
            if (placar.match.status==="inProgress"){
                reBootPlacar();
            }
            break;
        default:
            console.log("/////********recieved Action no reconocida", msg.payload.action);
            node.warn("Acción no reconocida: " + msg.payload.action);
            break;
    }

    // Guardar el estado actualizado

    flow.set('placar', placar);
    //Logs for Debug//////////////////////////////////////////////////
    console.log("==========>scoreCounter = ",placar.match.scoring.counter, "| currentGame = ", placar.match.scoring.currentGame)
    console.log("histroricalActions ->  " , placar.history.actions, "historialScoring ->  ", placar.history.scores);

    msg.payload.placar = placar;
    return msg;

    /////////////////////////////////////////////////////////////////////
    ///////////////////////////FUNCIONES/////////////////////////////////
    /////////////////////////////////////////////////////////////////////

    // Función para crear un nuevo objeto `placar`
    function newPlacar(parameters) {
        function generateUniqueID() {
            return Date.now().toString(36) + Math.random().toString(36).substring(2);
        }
        const initialScoring = {
            counter: 0,
            currentSet: 0,
            currentGame: [0, 0],
            formattedGame: [0, 0],
            setsWins: [0, 0],
            sets: [[0, 0], [0, 0], [0, 0]],
            inTiebreak: false
        };
        let newPlacar = {
            match : {
                matchID: generateUniqueID(),
                startTime: Date.now(),
                status: "inProgress",
                scoring: {... initialScoring},
            },
            params : { ...parameters },
            history: {
                actions: [{action: "startNewGame"}],
                scores: [JSON.parse(JSON.stringify(initialScoring))]
            },
            command: null
        };

        return newPlacar;

    }

    //////////////////////////////////////////////////////
    function reBootPlacar() {
        placar = newPlacar(parameters); // Reiniciar el marcador con parámetros por defecto
        flow.set('placar', placar);
    }
    //////////////////////////////////////////////////////
    function resetScore() {
        placar.match.scoring.counter += 1;          // Incremento el contador
        placar.match.scoring.currentSet = 0;
        placar.match.scoring.currentGame = [0, 0];
        placar.match.scoring.formattedGame = [0, 0];
        placar.match.scoring.setsWins = [0, 0];
        placar.match.scoring.sets = [[0, 0], [0, 0], [0, 0]];
        placar.match.scoring.inTiebreak = false;  

        placar.history.actions.push({ action: "Reset" });
        placar.history.scores.push(JSON.parse(JSON.stringify(placar.match.scoring)));
    }

    //////////////////////////////////////////////////////
    function undoScore() {
        if (placar.history.actions.length > 1 && placar.history.scores.length > 1){
            // Elimina la última acción y estado del historial
            let deletedAction = placar.history.actions.pop();
            let deletedScore = placar.history.scores.pop();
            //rescato el último valor del historial de scoring
            let lastScore = placar.history.scores[placar.history.scores.length-1]

            console.log("action deleted  -> ", deletedAction, "scoring deleted -> ",deletedScore," lastSCore -> ", lastScore);
            placar.match.scoring = JSON.parse(JSON.stringify(lastScore));
        }
    }

    //////////////////////////////////////////////////////
    function updateScore(teamIndex) {
        console.log(`------------------updating---------->`, teamIndex);
        placar.match.scoring.counter += 1;          // Incremento el contador
        placar.match.scoring.currentGame[teamIndex] += 1;          // Incremento el contador
        
        // Registro los historiales
        placar.history.actions.push({ action: teamIndex === 0 ? "pointA" : "pointB" });
        placar.history.scores.push(JSON.parse(JSON.stringify(placar.match.scoring)));

    }
