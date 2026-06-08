const DEFAULT_API = localStorage.getItem("astrobandApi") || "http://localhost";
const MQTT_URL = "wss://broker.hivemq.com:8884/mqtt";
const TOPIC_TELEMETRIA = "fiap/iot/astroband/telemetria";

const elements = {
  connection: document.getElementById("connectionStatus"),
  apiForm: document.getElementById("apiForm"),
  apiHost: document.getElementById("apiHost"),
  endpointLabel: document.getElementById("endpointLabel"),
  temperatureValue: document.getElementById("temperatureValue"),
  temperatureStatus: document.getElementById("temperatureStatus"),
  heartValue: document.getElementById("heartValue"),
  heartStatus: document.getElementById("heartStatus"),
  locationValue: document.getElementById("locationValue"),
  locationStatus: document.getElementById("locationStatus"),
  generalStatus: document.getElementById("generalStatus"),
  statusDescription: document.getElementById("statusDescription"),
  emergencyStatus: document.getElementById("emergencyStatus"),
  emergencyCard: document.getElementById("emergencyCard"),
  riskCard: document.getElementById("riskCard"),
  lastUpdate: document.getElementById("lastUpdate"),
  feed: document.getElementById("messageFeed"),
  chart: document.getElementById("telemetryChart")
};

const historico = {
  temperatura: [],
  batimentos: []
};

let apiBase = normalizarBaseUrl(DEFAULT_API);
let pollingId = null;
let mqttClient = null;

function normalizarBaseUrl(value) {
  const trimmed = String(value || "").trim().replace(/\/$/, "");

  if (!trimmed) {
    return "http://localhost";
  }

  if (trimmed.startsWith("http://") || trimmed.startsWith("https://")) {
    return trimmed;
  }

  return `http://${trimmed}`;
}

function setConnection(status, text) {
  elements.connection.classList.remove("connected", "error");

  if (status) {
    elements.connection.classList.add(status);
  }

  elements.connection.lastChild.nodeValue = ` ${text}`;
}

function formatNumber(value, decimals = 1) {
  return Number(value).toFixed(decimals);
}

function updatePill(element, text, type) {
  element.className = `pill ${type || ""}`.trim();
  element.textContent = text;
}

function statusToClass(status) {
  if (status === "normal" || status === "safe") {
    return "ok";
  }

  if (status === "critical" || status === "danger" || status === "emergency") {
    return "danger";
  }

  return "warn";
}

function statusToLabel(status) {
  const labels = {
    normal: "Normal",
    safe: "Zona segura",
    attention: "Atencao",
    critical: "Critico",
    danger: "Fora da zona",
    emergency: "Emergencia",
    unknown: "Sem leitura"
  };

  return labels[status] || "Aguardando";
}

function addFeedItem(text) {
  const item = document.createElement("li");
  const time = new Date().toLocaleTimeString("pt-BR", {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit"
  });

  item.textContent = `${time} | ${text}`;
  elements.feed.prepend(item);

  while (elements.feed.children.length > 5) {
    elements.feed.removeChild(elements.feed.lastElementChild);
  }
}

function pushHistory(key, value) {
  historico[key].push(Number(value));

  if (historico[key].length > 20) {
    historico[key].shift();
  }
}

function drawChart() {
  const canvas = elements.chart;
  const ctx = canvas.getContext("2d");
  const width = canvas.width;
  const height = canvas.height;
  const padding = 42;
  const graphWidth = width - padding * 2;
  const graphHeight = height - padding * 2;

  ctx.clearRect(0, 0, width, height);
  ctx.fillStyle = "#fbfdff";
  ctx.fillRect(0, 0, width, height);

  ctx.strokeStyle = "#dce3ed";
  ctx.lineWidth = 1;

  for (let i = 0; i <= 4; i++) {
    const y = padding + (graphHeight / 4) * i;
    ctx.beginPath();
    ctx.moveTo(padding, y);
    ctx.lineTo(width - padding, y);
    ctx.stroke();
  }

  drawLine(ctx, historico.temperatura, "#dc2626", 34, 41, padding, graphWidth, graphHeight);
  drawLine(ctx, historico.batimentos, "#2563eb", 55, 135, padding, graphWidth, graphHeight);

  ctx.fillStyle = "#18202f";
  ctx.font = "bold 15px Arial";
  ctx.fillText("Temperatura", padding, 24);
  ctx.fillStyle = "#dc2626";
  ctx.fillRect(padding + 102, 14, 12, 12);
  ctx.fillStyle = "#18202f";
  ctx.fillText("Batimentos", padding + 136, 24);
  ctx.fillStyle = "#2563eb";
  ctx.fillRect(padding + 224, 14, 12, 12);
}

function drawLine(ctx, values, color, min, max, padding, graphWidth, graphHeight) {
  if (values.length < 2) {
    return;
  }

  ctx.strokeStyle = color;
  ctx.lineWidth = 4;
  ctx.lineJoin = "round";
  ctx.lineCap = "round";
  ctx.beginPath();

  values.forEach((value, index) => {
    const x = padding + (graphWidth / Math.max(values.length - 1, 1)) * index;
    const normalized = Math.max(0, Math.min(1, (value - min) / (max - min)));
    const y = padding + graphHeight - normalized * graphHeight;

    if (index === 0) {
      ctx.moveTo(x, y);
    } else {
      ctx.lineTo(x, y);
    }
  });

  ctx.stroke();
}

function updateDashboard(data) {
  const temperatura = Number(data.temperature);
  const batimentos = Number(data.heartbeat);
  const distancia = Number(data.distance);
  const emergencia = Boolean(data.emergency);
  const missionStatus = String(data.status || (emergencia ? "emergency" : "normal"));

  elements.temperatureValue.textContent = `${formatNumber(temperatura)} C`;
  elements.heartValue.textContent = `${Math.round(batimentos)} bpm`;
  elements.locationValue.textContent = `${Math.round(distancia)} cm`;

  const tempStatus = temperatura >= 38 ? "critical" : temperatura < 35.5 ? "attention" : "normal";
  const heartStatus = batimentos > 120 ? "critical" : batimentos > 100 || batimentos < 60 ? "attention" : "normal";
  const locationStatus = distancia > 220 ? "danger" : distancia > 150 ? "attention" : "safe";

  updatePill(elements.temperatureStatus, statusToLabel(tempStatus), statusToClass(tempStatus));
  updatePill(elements.heartStatus, statusToLabel(heartStatus), statusToClass(heartStatus));
  updatePill(elements.locationStatus, statusToLabel(locationStatus), statusToClass(locationStatus));

  elements.riskCard.classList.toggle("alert", missionStatus !== "normal");
  elements.emergencyCard.classList.toggle("alert", emergencia);
  elements.generalStatus.textContent = missionStatus === "normal" ? "Missao estavel" : statusToLabel(missionStatus);
  elements.emergencyStatus.textContent = emergencia ? "SOS acionado" : "Nao acionado";
  elements.statusDescription.textContent = missionStatus === "normal"
    ? "Temperatura, batimentos e localizacao estao dentro do esperado."
    : "Algum indicador saiu da zona segura simulada e exige acompanhamento da equipe.";

  elements.lastUpdate.textContent = `Atualizado as ${new Date().toLocaleTimeString("pt-BR")}`;

  pushHistory("temperatura", temperatura);
  pushHistory("batimentos", batimentos);
  drawChart();

  addFeedItem(`${formatNumber(temperatura)} C, ${Math.round(batimentos)} bpm, ${Math.round(distancia)} cm`);
}

async function fetchAstronaut() {
  try {
    const response = await fetch(`${apiBase}/astronaut`, { cache: "no-store" });

    if (!response.ok) {
      throw new Error("Resposta invalida da API");
    }

    const data = await response.json();
    setConnection("connected", "API conectada");
    updateDashboard(data);
  } catch (error) {
    setConnection("error", "API indisponivel");
  }
}

function startPolling() {
  clearInterval(pollingId);
  if (mqttClient) {
    mqttClient.end(true);
    mqttClient = null;
  }
  elements.endpointLabel.textContent = `${apiBase}/astronaut`;
  localStorage.setItem("astrobandApi", apiBase);
  fetchAstronaut();
  pollingId = setInterval(fetchAstronaut, 2000);
}

function startMqtt() {
  if (!window.mqtt) {
    setConnection("error", "MQTT indisponivel");
    addFeedItem("Biblioteca MQTT nao carregou no navegador");
    return;
  }

  clearInterval(pollingId);
  elements.endpointLabel.textContent = `MQTT: ${TOPIC_TELEMETRIA}`;
  setConnection("", "Conectando MQTT");

  mqttClient = mqtt.connect(MQTT_URL, {
    clientId: `astroband-dashboard-${Math.random().toString(16).slice(2)}`,
    clean: true,
    connectTimeout: 5000,
    reconnectPeriod: 3000
  });

  mqttClient.on("connect", () => {
    setConnection("connected", "MQTT conectado");
    mqttClient.subscribe(TOPIC_TELEMETRIA);
    addFeedItem("Dashboard conectado ao Wokwi por MQTT");
  });

  mqttClient.on("reconnect", () => setConnection("", "Reconectando MQTT"));
  mqttClient.on("error", () => setConnection("error", "Erro MQTT"));

  mqttClient.on("message", (topic, payload) => {
    if (topic !== TOPIC_TELEMETRIA) {
      return;
    }

    try {
      updateDashboard(JSON.parse(payload.toString()));
    } catch (error) {
      addFeedItem("Mensagem MQTT fora do formato JSON");
    }
  });
}

elements.apiForm.addEventListener("submit", (event) => {
  event.preventDefault();
  apiBase = normalizarBaseUrl(elements.apiHost.value);
  startPolling();
  addFeedItem(`Conectando em ${apiBase}`);
});

elements.apiHost.value = apiBase;
drawChart();
startMqtt();
