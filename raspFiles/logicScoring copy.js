function newMatch() {
    function generateUniqueID() {
        return Date.now().toString(36) + Math.random().toString(36).substring(2);
    }
    return {
        matchID: generateUniqueID(),  // Generar un ID único para el partido
        startTime: msg.timestamp,
        matchStatus: global.get("globalMatchStatus") || "inProgress",
        score: {
            sets: [[0, 0], [0, 0], [0, 0]],  // Array para almacenar los juegos ganados en cada set
            setsWins: [0, 0],                 // Indica el resultado de sets ganados
            currentSet: 0,                  // Índice del set actual en el array de sets
            currentGame: [0, 0],            // Puntuación del juego actual
            gameFormated: [0, 0],            // Formateo de puntuación del game actual
            inTiebreak: false,               // Flag para indicar si el juego actual es un tiebreak
        },
        parameters: {
            maxPoint: global.get("globalMaxPoint") || 9,
            gameMode:"A",
            deviceID:"deviceID",
            courtID:"courtID",
            clubID:"clubID",
            network: {
                ssid:"PadelClubWifi",
                pass:"secureWifiPass"
            },

        },
        actionHistory: [],  // Historial de todas las acciones realizadas
        scoreHistory: []  // Historial de los cambios de puntuación para poder deshacer cambios
    };
}

function updateScore(teamIndex) {
    console.log(`------------------updating`);
    let cG = quadra.match.score.currentGame;
    let cSI = quadra.match.score.currentSet;
    let cS = quadra.match.score.sets[cSI];
    let setsWins = quadra.match.score.setsWins;
    let inTiebreak = quadra.match.score.inTiebreak;
    let maxPoint = quadra.match.parameters.maxPoint;

    quadra.match.actionHistory.push({ action: teamIndex === 0 ? "pointA" : "pointB" });
    quadra.match.scoreHistory.push(JSON.parse(JSON.stringify(quadra.match.score)));

    cG[teamIndex] += 1;
    console.log("----->cs:", cS, " cSI:", cSI, " cGame:", cG);

    if (maxPoint === 9) {
        handleSingleSetGame(cG, cS, cSI, setsWins, inTiebreak, teamIndex);
    } else if (maxPoint === 7) {
        handleThreeSetGame(cG, cS, cSI, setsWins, inTiebreak,teamIndex);
    }
}

function handleSingleSetGame(cG, cS, cSI, setsWins, inTiebreak, teamIndex) {
    if (inTiebreak) {
        console.log("---Estoy en Tiebreak--> ");
        if ((cG[0] >= 7 || cG[1] >= 7) && Math.abs(cG[0] - cG[1]) >= 2) {
            console.log("Condición ganador tiebreak ");
            cS[teamIndex] += 1;
            setsWins[teamIndex] += 1;
            cG = [0, 0];
            inTiebreak = false;
            quadra.match.matchStatus = "completed";
            console.log("---Finalizó Tiebreak--> ", "ganó el set el team: ", teamIndex);
        }
    } else {
        if ((cG[0] >= 4 || cG[1] >= 4) && Math.abs(cG[0] - cG[1]) >= 2) {
            cS[teamIndex] += 1;
            if (cS[0] === 8 && cS[1] === 8) {
                inTiebreak = true;
                cG = [0, 0];
            }
            if (cS[teamIndex] >= 9 && Math.abs(cS[0] - cS[1]) >= 2) {
                setsWins[teamIndex] += 1;
                quadra.match.matchStatus = "completed";
                console.log("Ganador del set el team: ", teamIndex);
            }
            cG = [0, 0];
        }
    }
    quadra.match.score.currentGame = cG;
    quadra.match.score.currentSet = cSI;
    quadra.match.score.sets[cSI] = cS;
    quadra.match.score.setsWins = setsWins;
    quadra.match.score.inTiebreak = inTiebreak;
    quadra.match.score.gameFormated = inTiebreak ? cG : formatGame(cG);
}

function handleThreeSetGame(cG, cS, cSI, setsWins,inTiebreak, teamIndex) {
    if (cSI <= 1){
            if (inTiebreak) {
                console.log( "---Estoy en Tiebreak--> ");
                // Manejar puntuación en tiebreak: simplemente incrementar puntos
                if ((cG[0] >= 7 || cG[1] >= 7) && Math.abs(cG[0] - cG[1]) >= 2) { // Condición para ganar el tiebreak
                        console.log("Condición ganador tiebreak ")
                    cS[teamIndex] += 1;
                    cSI +=1;
                    setsWins[teamIndex] += 1;
                    cS = [0 , 0];
                        console.log(`incremento de indice me muevo al siguiente SET`);
                    
                    cG = [0, 0]; // Reiniciar juego
                    inTiebreak = false; // Salir del modo tiebreak
                        console.log( "---Finalizó Tiebreak--> ", "ganó el set el team : ",teamIndex);
                }
            } else {
                // Verificar si hay un ganador del Game
                if ((cG[0] >= 4 || cG[1] >= 4) && Math.abs(cG[0] - cG[1]) >= 2) {
                    //sumo 1 al set correspondiente y 
                        cS[teamIndex]+=1;
                    // Verificar si hay un ganador del set
                    if (cS[0] === 6 && cS[1] === 6) { // Comprobar si se necesita iniciar un tiebreak
                        inTiebreak = true;
                        //cG = [0, 0]; // Reiniciar puntuación para tiebreak
                    }
                    if (cS[teamIndex] >= 6 && Math.abs(cS[0] - cS[1]) >= 2) {
                        //if (cSI < cS.length - 1) {
                        if (cSI <= cS.length) {
                            cSI +=1;
                            setsWins[teamIndex] += 1;
                            cS = [0 , 0];
                                console.log(`incremento de indice me muevo al siguiente SET`);
                        }
                    }
                    cG = [0, 0];  // Reiniciar el juego actual
                }
            }
    }else{ //SuperTiebreak condition
        inTiebreak = true;
        if ((cG[0] >= 10 || cG[1] >= 10) && Math.abs(cG[0] - cG[1]) >= 2) { // Condición para ganar el tiebreak
            //cS[teamIndex] += 1;
            cS[0] = cG[0];
            cS[1] = cG[1];
            setsWins[teamIndex] += 1;
            cG = [0, 0];                                        // Reiniciar juego
            inTiebreak = false; // Salir del modo tiebreak
                console.log( "---Finalizó Tiebreak--> ", "ganó el set el team : ",teamIndex);
        }

    }
    
    if (setsWins[0] === 2 || setsWins[1] === 2) {
        // Alguno de los equipos ha ganado el partido
        console.log("El partido ha terminado. Un equipo ha ganado 2 sets.");
        quadra.match.matchStatus = "completed";
    } 
    quadra.match.score.inTiebreak = inTiebreak;
    quadra.match.score.currentGame = cG;
    quadra.match.score.currentSet = cSI;
    quadra.match.score.sets[cSI] = cS;
    quadra.match.score.setsWins = setsWins;
    quadra.match.score.gameFormated = inTiebreak ? cG : formatGame(cG);
}

function formatGame(gameStatus) {
    let pointsA = JSON.parse(JSON.stringify(gameStatus[0]));
    let pointsB = JSON.parse(JSON.stringify(gameStatus[1]));

    if (pointsA >= 3 && pointsB >= 3) {
        console.log("pointsA y pointsB mayores que 3");
        if (pointsA === pointsB) {
            console.log("pointsA === pointsB retorna 40-40");
            return ["40", "40"];  // Deuce
        }
        if (pointsA > pointsB) {
            console.log("pointsA > pointsB retorna AD-40");
            return ["AD", "40"];
        }
        if (pointsB > pointsA) {
            console.log("pointsA < pointsB retorna 40-AD");
            return ["40", "AD"];
        }
    } else {
        console.log("pointsA & pointsB menor a 3 entonces");
        console.log([formatPoint(pointsA), formatPoint(pointsB)]);
        return [formatPoint(pointsA), formatPoint(pointsB)];
    }

    function formatPoint(point) {
        return point === 0 ? "0" :
            point === 1 ? "15" :
                point === 2 ? "30" :
                    point === 3 ? "40" :
                        "AD";
    }
}

// Obtener el objeto 'quadra' del contexto de flujo
let quadra = flow.get('quadra') || { match: newMatch() };
// Evaluar la acción proveniente del nodo inject
switch (msg.payload.action) {
    case "A":
        if (quadra.match.matchStatus === "inProgress") {
            updateScore(0);
        }
        break;
    case "B":
        if (quadra.match.matchStatus === "inProgress") {
            updateScore(1);
        }
        break;
    case "Undo":
        let aH = quadra.match.actionHistory;
        let sH = quadra.match.scoreHistory;
        if (aH.length > 0 && sH.length > 0) {
            quadra.match.actionHistory.pop();
            let previousState = quadra.match.scoreHistory.pop();
            console.log("Undo - Restoring to previous state:", previousState);
            quadra.match.score = JSON.parse(JSON.stringify(previousState));
        }
        break;
    case "Reset":
        quadra.match = newMatch();
        break;
    default:
        node.warn("Acción no reconocida: " + msg.payload.action);
        break;
}

// Guardar el estado actualizado
flow.set('quadra', quadra);
msg.payload.quadra = quadra;
return msg;
