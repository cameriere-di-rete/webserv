// Common utility functions for test pages

export function renderStatus(responseDiv, status, statusText, statusClass) {
  const statusSpan = document.createElement("span");
  statusSpan.className = statusClass;
  statusSpan.textContent = `Status: ${status} ${statusText}`;
  responseDiv.appendChild(statusSpan);
}

export function renderError(responseDiv, message) {
  responseDiv.innerHTML = "";
  const errorSpan = document.createElement("span");
  errorSpan.className = "status-error";
  errorSpan.textContent = `Error: ${message}`;
  responseDiv.appendChild(errorSpan);
  responseDiv.classList.add("show");
}

export function renderPre(responseDiv, label, text) {
  if (label) {
    const labelDiv = document.createElement("div");
    labelDiv.textContent = label;
    responseDiv.appendChild(labelDiv);
  }
  const pre = document.createElement("pre");
  pre.textContent = text || "(empty)";
  responseDiv.appendChild(pre);
}
