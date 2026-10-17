/**
 * ProcessScope Dashboard Frontend Logic
 * Course: Operating Systems and Systems Programming
 *
 * Implements real-time /proc telemetry polling, sorting, filtering,
 * zombie alerts, and modal inspection.
 */

document.addEventListener('DOMContentLoaded', () => {
  // State variables
  let processes = [];
  let sortColumn = 'pid';
  let sortAsc = true;
  let autoRefreshTimer = null;
  let isFetching = false;

  // DOM Elements
  const valTotal = document.getElementById('valTotal');
  const valZombies = document.getElementById('valZombies');
  const valAdopted = document.getElementById('valAdopted');
  const valStandard = document.getElementById('valStandard');

  const zombieAlertBanner = document.getElementById('zombieAlertBanner');
  const zombieAlertTitle = document.getElementById('zombieAlertTitle');
  const zombieAlertDesc = document.getElementById('zombieAlertDesc');

  const searchInput = document.getElementById('searchInput');
  const clearSearchBtn = document.getElementById('clearSearchBtn');
  const classificationFilter = document.getElementById('classificationFilter');
  const stateFilter = document.getElementById('stateFilter');
  const filteredStats = document.getElementById('filteredStats');

  const processTableBody = document.getElementById('processTableBody');
  const tableHeaders = document.querySelectorAll('.process-table th[data-sort]');
  const refreshBtn = document.getElementById('refreshBtn');
  const refreshIcon = refreshBtn.querySelector('.refresh-icon');
  const autoRefreshCheckbox = document.getElementById('autoRefreshCheckbox');

  const connectionBadge = document.getElementById('connectionBadge');
  const connectionStatus = document.getElementById('connectionStatus');
  const lastScanTime = document.getElementById('lastScanTime');

  const detailModal = document.getElementById('detailModal');
  const modalTitle = document.getElementById('modalTitle');
  const modalBody = document.getElementById('modalBody');
  const closeDetailModalBtn = document.getElementById('closeDetailModalBtn');

  const theoryModal = document.getElementById('theoryModal');
  const openTheoryModalBtn = document.getElementById('openTheoryModalBtn');
  const closeTheoryModalBtn = document.getElementById('closeTheoryModalBtn');

  /**
   * Fetches latest process table from local Python API.
   */
  async function fetchProcesses() {
    if (isFetching) return;
    isFetching = true;
    refreshIcon.classList.add('spinning');

    try {
      const response = await fetch('/api/processes', { cache: 'no-store' });
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }

      const data = await response.json();
      if (data.error) {
        throw new Error(data.error);
      }

      processes = data.processes || [];

      // Update Metrics
      updateMetrics(data.counts || {});

      // Update Connection Status
      setConnectionStatus(true);

      // Update Timestamp
      if (data.timestamp) {
        const d = new Date(data.timestamp * 1000);
        lastScanTime.textContent = d.toLocaleTimeString();
      }

      // Render Table
      renderTable();

    } catch (err) {
      console.error('Fetch error:', err);
      setConnectionStatus(false, err.message);
      if (processes.length === 0) {
        renderErrorState(err.message);
      }
    } finally {
      isFetching = false;
      refreshIcon.classList.remove('spinning');
    }
  }

  /**
   * Updates top metric cards and zombie alert banner.
   */
  function updateMetrics(counts) {
    const total = counts.total ?? processes.length;
    const zombies = counts.zombies ?? processes.filter(p => p.classification === 'zombie').length;
    const adopted = counts.adopted_candidates ?? processes.filter(p => p.classification === 'adopted_candidate').length;
    const standard = counts.standard ?? (total - zombies - adopted);

    valTotal.textContent = total;
    valZombies.textContent = zombies;
    valAdopted.textContent = adopted;
    valStandard.textContent = standard;

    // Zombie alert banner
    if (zombies > 0) {
      zombieAlertBanner.style.display = 'flex';
      zombieAlertTitle.textContent = `${zombies} Active Zombie Process${zombies > 1 ? 'es' : ''} Detected!`;
      zombieAlertDesc.textContent = `Found ${zombies} process(es) in state 'Z'. The parent has terminated without wait()/waitpid() or is delaying reaping.`;
    } else {
      zombieAlertBanner.style.display = 'none';
    }
  }

  /**
   * Updates live connection indicator.
   */
  function setConnectionStatus(isLive, message = '') {
    const dot = connectionBadge.querySelector('.status-dot');
    if (isLive) {
      dot.className = 'status-dot live';
      connectionStatus.textContent = 'Live';
      connectionBadge.title = 'Connected to C scanner backend';
    } else {
      dot.className = 'status-dot error';
      connectionStatus.textContent = 'Offline';
      connectionBadge.title = message || 'Connection lost';
    }
  }

  /**
   * Filters and sorts process list.
   */
  function getFilteredAndSortedProcesses() {
    const query = searchInput.value.trim().toLowerCase();
    const classFilter = classificationFilter.value;
    const stFilter = stateFilter.value;

    let filtered = processes.filter(p => {
      // Search by PID or Name
      if (query) {
        const matchPid = p.pid.toString().includes(query);
        const matchName = p.name.toLowerCase().includes(query);
        if (!matchPid && !matchName) return false;
      }

      // Classification Filter
      if (classFilter !== 'all' && p.classification !== classFilter) {
        return false;
      }

      // State Filter
      if (stFilter !== 'all' && p.state !== stFilter) {
        return false;
      }

      return true;
    });

    // Sort
    filtered.sort((a, b) => {
      let valA = a[sortColumn];
      let valB = b[sortColumn];

      if (typeof valA === 'string') {
        valA = valA.toLowerCase();
        valB = valB.toLowerCase();
      }

      if (valA < valB) return sortAsc ? -1 : 1;
      if (valA > valB) return sortAsc ? 1 : -1;
      return 0;
    });

    return filtered;
  }

  /**
   * Renders the process table rows.
   */
  function renderTable() {
    const filtered = getFilteredAndSortedProcesses();
    filteredStats.textContent = `Showing ${filtered.length} of ${processes.length}`;

    if (filtered.length === 0) {
      processTableBody.innerHTML = `
        <tr>
          <td colspan="6" class="state-cell">
            <div>No matching processes found.</div>
            <div style="font-size:0.75rem; margin-top:0.25rem;">Try adjusting your search query or filters.</div>
          </td>
        </tr>
      `;
      return;
    }

    const rowsHtml = filtered.map(p => {
      const isZombie = p.classification === 'zombie';
      const rowClass = isZombie ? 'row-zombie' : '';

      // State badge
      let stateBadgeClass = 'badge-state-s';
      if (p.state === 'Z') stateBadgeClass = 'badge-state-z';
      else if (p.state === 'R') stateBadgeClass = 'badge-state-r';

      // Classification badge
      let classBadge = '<span class="badge badge-class-standard">Standard</span>';
      if (p.classification === 'zombie') {
        classBadge = '<span class="badge badge-class-zombie">ZOMBIE</span>';
      } else if (p.classification === 'adopted_candidate') {
        classBadge = '<span class="badge badge-class-adopted">Adopted Candidate</span>';
      }

      const escapedName = escapeHtml(p.name);

      return `
        <tr class="${rowClass}">
          <td class="mono-cell">${p.pid}</td>
          <td class="mono-cell">${p.ppid}</td>
          <td>
            <span class="badge ${stateBadgeClass}" title="${escapeHtml(p.state_desc || '')}">
              ${p.state}
            </span>
          </td>
          <td>${classBadge}</td>
          <td class="mono-cell" title="${escapedName}">${escapedName}</td>
          <td>
            <button class="btn-inspect" data-pid="${p.pid}">Inspect</button>
          </td>
        </tr>
      `;
    }).join('');

    processTableBody.innerHTML = rowsHtml;

    // Attach inspect listeners
    const inspectBtns = processTableBody.querySelectorAll('.btn-inspect');
    inspectBtns.forEach(btn => {
      btn.addEventListener('click', (e) => {
        const pid = parseInt(e.currentTarget.getAttribute('data-pid'), 10);
        const proc = processes.find(p => p.pid === pid);
        if (proc) {
          openDetailModal(proc);
        }
      });
    });
  }

  function renderErrorState(message) {
    processTableBody.innerHTML = `
      <tr>
        <td colspan="6" class="state-cell" style="color: var(--color-danger);">
          <div style="font-weight: 600; margin-bottom: 0.25rem;">Unable to connect to ProcessScope scanner</div>
          <div style="font-size: 0.8rem; color: var(--text-muted);">${escapeHtml(message)}</div>
          <div style="margin-top: 1rem;">
            <button class="btn btn-secondary" onclick="window.location.reload()">Retry Connection</button>
          </div>
        </td>
      </tr>
    `;
  }

  /**
   * Opens the process inspection modal with conceptual explanation.
   */
  function openDetailModal(p) {
    modalTitle.textContent = `Process Details — PID ${p.pid} (${p.name})`;

    let explanation = '';
    if (p.classification === 'zombie') {
      explanation = `
        <strong style="color: var(--color-danger);">Zombie Analysis:</strong>
        This process has executed <code>exit()</code> and finished execution.
        Its memory pages and files are freed, but its Process Control Block (PCB) remains in the kernel
        table awaiting its parent process (PID ${p.ppid}) to call <code>wait()</code> or <code>waitpid()</code>.
      `;
    } else if (p.classification === 'adopted_candidate') {
      explanation = `
        <strong style="color: var(--color-warning);">Adopted Candidate Analysis:</strong>
        This process currently has PPID 1. In Linux, when a parent dies, its children are reparented
        to init (PID 1) or an active subreaper process. Note: A single snapshot cannot distinguish
        whether this process was originally spawned as a system daemon by PID 1 or was reparented after orphanhood.
      `;
    } else {
      explanation = `
        <strong>Standard Process:</strong>
        Active process functioning with active parent PID ${p.ppid}.
      `;
    }

    modalBody.innerHTML = `
      <div class="detail-row">
        <span class="detail-label">Process ID (PID):</span>
        <span class="detail-value">${p.pid}</span>
      </div>
      <div class="detail-row">
        <span class="detail-label">Parent PID (PPID):</span>
        <span class="detail-value">${p.ppid}</span>
      </div>
      <div class="detail-row">
        <span class="detail-label">Executable Name:</span>
        <span class="detail-value">${escapeHtml(p.name)}</span>
      </div>
      <div class="detail-row">
        <span class="detail-label">Linux Process State:</span>
        <span class="detail-value">${p.state} (${escapeHtml(p.state_desc || 'Unknown')})</span>
      </div>
      <div class="detail-row">
        <span class="detail-label">Classification:</span>
        <span class="detail-value">${escapeHtml(p.classification)}</span>
      </div>
      <div class="detail-explanation">
        ${explanation}
      </div>
    `;

    detailModal.style.display = 'flex';
  }

  function closeDetailModal() {
    detailModal.style.display = 'none';
  }

  function openTheoryModal() {
    theoryModal.style.display = 'flex';
  }

  function closeTheoryModal() {
    theoryModal.style.display = 'none';
  }

  function escapeHtml(str) {
    if (!str) return '';
    return str
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#039;');
  }

  // Event Listeners
  refreshBtn.addEventListener('click', fetchProcesses);

  searchInput.addEventListener('input', () => {
    clearSearchBtn.style.display = searchInput.value ? 'block' : 'none';
    renderTable();
  });

  clearSearchBtn.addEventListener('click', () => {
    searchInput.value = '';
    clearSearchBtn.style.display = 'none';
    renderTable();
  });

  classificationFilter.addEventListener('change', renderTable);
  stateFilter.addEventListener('change', renderTable);

  // Sorting header click
  tableHeaders.forEach(th => {
    th.addEventListener('click', () => {
      const col = th.getAttribute('data-sort');
      if (sortColumn === col) {
        sortAsc = !sortAsc;
      } else {
        sortColumn = col;
        sortAsc = true;
      }

      // Update arrow icons
      tableHeaders.forEach(h => {
        const arrow = h.querySelector('.sort-arrow');
        if (h === th) {
          arrow.textContent = sortAsc ? ' ▲' : ' ▼';
        } else {
          arrow.textContent = '';
        }
      });

      renderTable();
    });
  });

  // Modals
  closeDetailModalBtn.addEventListener('click', closeDetailModal);
  closeTheoryModalBtn.addEventListener('click', closeTheoryModal);
  openTheoryModalBtn.addEventListener('click', openTheoryModal);

  window.addEventListener('click', (e) => {
    if (e.target === detailModal) closeDetailModal();
    if (e.target === theoryModal) closeTheoryModal();
  });

  window.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
      closeDetailModal();
      closeTheoryModal();
    }
  });

  // Auto-refresh control
  function startAutoRefresh() {
    stopAutoRefresh();
    autoRefreshTimer = setInterval(() => {
      // Don't auto-refresh if modal is open or user is actively typing in search
      if (detailModal.style.display === 'flex' || theoryModal.style.display === 'flex') {
        return;
      }
      fetchProcesses();
    }, 3000);
  }

  function stopAutoRefresh() {
    if (autoRefreshTimer) {
      clearInterval(autoRefreshTimer);
      autoRefreshTimer = null;
    }
  }

  autoRefreshCheckbox.addEventListener('change', () => {
    if (autoRefreshCheckbox.checked) {
      startAutoRefresh();
    } else {
      stopAutoRefresh();
    }
  });

  // Initial load
  fetchProcesses();
  if (autoRefreshCheckbox.checked) {
    startAutoRefresh();
  }
});
