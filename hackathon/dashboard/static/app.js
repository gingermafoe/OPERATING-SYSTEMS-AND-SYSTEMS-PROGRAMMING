/**
 * ProcessScope Dashboard Frontend Logic
 * Course: Operating Systems and Systems Programming
 *
 * Implements real-time /proc telemetry polling, sorting, filtering,
 * zombie alerts, and modal inspection with the cinematic cyber design.
 */

document.addEventListener('DOMContentLoaded', () => {
  // Fast, non-blocking Boot Sequence Dismissal
  const bootSequence = document.getElementById('bootSequence');
  if (bootSequence) {
    setTimeout(() => {
      bootSequence.classList.add('dismissed');
      setTimeout(() => {
        bootSequence.style.display = 'none';
      }, 500);
    }, 900);

    // Dismiss immediately on user click or keypress
    window.addEventListener('click', () => {
      bootSequence.classList.add('dismissed');
    }, { once: true });
    window.addEventListener('keydown', () => {
      bootSequence.classList.add('dismissed');
    }, { once: true });
  }

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
  const tableHeaders = document.querySelectorAll('.cyber-table th[data-sort]');
  const refreshBtn = document.getElementById('refreshBtn');
  const refreshIcon = refreshBtn ? refreshBtn.querySelector('.refresh-svg, .refresh-icon') : null;
  const autoRefreshCheckbox = document.getElementById('autoRefreshCheckbox');

  const statusPulseDot = document.getElementById('statusPulseDot');
  const statusText = document.getElementById('statusText');
  const lastScanTime = document.getElementById('lastScanTime');

  const detailModal = document.getElementById('detailModal');
  const modalTitle = document.getElementById('modalTitle');
  const modalBody = document.getElementById('modalBody');
  const closeDetailModalBtn = document.getElementById('closeDetailModalBtn');

  // Navigation active highlighting on scroll
  const navLinks = document.querySelectorAll('.nav-link');
  const sections = document.querySelectorAll('section[id]');

  window.addEventListener('scroll', () => {
    let current = '';
    const scrollY = window.pageYOffset;

    sections.forEach(section => {
      const sectionHeight = section.offsetHeight;
      const sectionTop = section.offsetTop - 120;
      if (scrollY >= sectionTop && scrollY < sectionTop + sectionHeight) {
        current = section.getAttribute('id');
      }
    });

    navLinks.forEach(link => {
      link.classList.remove('active');
      if (link.getAttribute('href') === `#${current}`) {
        link.classList.add('active');
      }
    });
  });

  /**
   * Fetches latest process table from local Python API.
   */
  async function fetchProcesses() {
    if (isFetching) return;
    isFetching = true;
    if (refreshIcon) refreshIcon.classList.add('spinning');

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
        if (lastScanTime) lastScanTime.textContent = d.toLocaleTimeString();
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
      if (refreshIcon) refreshIcon.classList.remove('spinning');
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

    if (valTotal) valTotal.textContent = total;
    if (valZombies) valZombies.textContent = zombies;
    if (valAdopted) valAdopted.textContent = adopted;
    if (valStandard) valStandard.textContent = standard;

    // Zombie alert banner
    if (zombieAlertBanner) {
      if (zombies > 0) {
        zombieAlertBanner.style.display = 'flex';
        if (zombieAlertTitle) zombieAlertTitle.textContent = `${zombies} Active Zombie Process${zombies > 1 ? 'es' : ''} Detected!`;
        if (zombieAlertDesc) zombieAlertDesc.textContent = `Found ${zombies} process(es) in state 'Z'. The parent has terminated without wait()/waitpid() or is delaying reaping.`;
      } else {
        zombieAlertBanner.style.display = 'none';
      }
    }
  }

  /**
   * Updates live connection indicator.
   */
  function setConnectionStatus(isLive, message = '') {
    if (statusPulseDot) {
      if (isLive) {
        statusPulseDot.className = 'status-pulse-dot live';
        if (statusText) statusText.textContent = 'LIVE /PROC';
      } else {
        statusPulseDot.className = 'status-pulse-dot error';
        if (statusText) statusText.textContent = 'OFFLINE';
      }
    }
  }

  /**
   * Filters and sorts process list.
   */
  function getFilteredAndSortedProcesses() {
    const query = searchInput ? searchInput.value.trim().toLowerCase() : '';
    const classFilter = classificationFilter ? classificationFilter.value : 'all';
    const stFilter = stateFilter ? stateFilter.value : 'all';

    let filtered = processes.filter(p => {
      // Search by PID or Name
      if (query) {
        const matchPid = p.pid.toString().includes(query);
        const matchPpid = p.ppid.toString().includes(query);
        const matchName = p.name.toLowerCase().includes(query);
        if (!matchPid && !matchPpid && !matchName) return false;
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
    if (!processTableBody) return;
    const filtered = getFilteredAndSortedProcesses();
    if (filteredStats) filteredStats.textContent = `Showing ${filtered.length} of ${processes.length}`;

    if (filtered.length === 0) {
      processTableBody.innerHTML = `
        <tr>
          <td colspan="6" class="table-loading-cell">
            <div style="font-weight: 600; color: var(--text-primary); margin-bottom: 0.35rem;">No matching processes found</div>
            <div style="font-size:0.78rem; color: var(--text-muted);">Try adjusting your search query, state, or classification filters.</div>
          </td>
        </tr>
      `;
      return;
    }

    const rowsHtml = filtered.map(p => {
      const isZombie = p.classification === 'zombie';
      const rowClass = isZombie ? 'row-zombie' : '';

      // State badge
      let stateBadgeClass = 'c-badge-state-s';
      if (p.state === 'Z') stateBadgeClass = 'c-badge-state-z';
      else if (p.state === 'R') stateBadgeClass = 'c-badge-state-r';

      // Classification badge
      let classBadge = '<span class="c-badge c-badge-class-standard">Standard</span>';
      if (p.classification === 'zombie') {
        classBadge = '<span class="c-badge c-badge-class-zombie">ZOMBIE</span>';
      } else if (p.classification === 'adopted_candidate') {
        classBadge = '<span class="c-badge c-badge-class-adopted">Adopted Candidate</span>';
      }

      const escapedName = escapeHtml(p.name);

      return `
        <tr class="${rowClass}">
          <td class="font-mono">${p.pid}</td>
          <td class="font-mono">${p.ppid}</td>
          <td>
            <span class="c-badge ${stateBadgeClass}" title="${escapeHtml(p.state_desc || '')}">
              ${p.state}
            </span>
          </td>
          <td>${classBadge}</td>
          <td class="font-mono" title="${escapedName}">${escapedName}</td>
          <td>
            <button class="btn-table-inspect" data-pid="${p.pid}">Inspect</button>
          </td>
        </tr>
      `;
    }).join('');

    processTableBody.innerHTML = rowsHtml;

    // Attach inspect listeners
    const inspectBtns = processTableBody.querySelectorAll('.btn-table-inspect');
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
    if (!processTableBody) return;
    processTableBody.innerHTML = `
      <tr>
        <td colspan="6" class="table-loading-cell" style="color: var(--color-danger);">
          <div style="font-weight: 700; margin-bottom: 0.35rem; color: #FFFFFF;">Unable to connect to ProcessScope scanner</div>
          <div style="font-size: 0.8rem; color: var(--text-muted);">${escapeHtml(message)}</div>
          <div style="margin-top: 1rem;">
            <button class="btn-table-inspect" onclick="window.location.reload()">Retry Connection</button>
          </div>
        </td>
      </tr>
    `;
  }

  /**
   * Opens the process inspection modal with conceptual explanation.
   */
  function openDetailModal(p) {
    if (!detailModal || !modalBody) return;
    if (modalTitle) modalTitle.textContent = `PROCESS // PID ${p.pid} (${p.name})`;

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
        <strong>Standard Active Task:</strong>
        Active process functioning with active parent PID ${p.ppid}.
      `;
    }

    modalBody.innerHTML = `
      <div class="cyber-detail-row">
        <span class="detail-k">Process ID (PID):</span>
        <span class="detail-v">${p.pid}</span>
      </div>
      <div class="cyber-detail-row">
        <span class="detail-k">Parent PID (PPID):</span>
        <span class="detail-v">${p.ppid}</span>
      </div>
      <div class="cyber-detail-row">
        <span class="detail-k">Command / Executable:</span>
        <span class="detail-v">${escapeHtml(p.name)}</span>
      </div>
      <div class="cyber-detail-row">
        <span class="detail-k">Linux Process State:</span>
        <span class="detail-v">${p.state} (${escapeHtml(p.state_desc || 'Unknown')})</span>
      </div>
      <div class="cyber-detail-row">
        <span class="detail-k">Classification:</span>
        <span class="detail-v">${escapeHtml(p.classification)}</span>
      </div>
      <div class="cyber-detail-box">
        ${explanation}
      </div>
    `;

    detailModal.style.display = 'flex';
  }

  function closeDetailModal() {
    if (detailModal) detailModal.style.display = 'none';
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
  if (refreshBtn) refreshBtn.addEventListener('click', fetchProcesses);

  if (searchInput) {
    searchInput.addEventListener('input', () => {
      if (clearSearchBtn) clearSearchBtn.style.display = searchInput.value ? 'block' : 'none';
      renderTable();
    });
  }

  if (clearSearchBtn) {
    clearSearchBtn.addEventListener('click', () => {
      if (searchInput) searchInput.value = '';
      clearSearchBtn.style.display = 'none';
      renderTable();
    });
  }

  if (classificationFilter) classificationFilter.addEventListener('change', renderTable);
  if (stateFilter) stateFilter.addEventListener('change', renderTable);

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

      // Update sort icons
      tableHeaders.forEach(h => {
        const icon = h.querySelector('.sort-icon');
        if (h === th) {
          icon.textContent = sortAsc ? ' ▲' : ' ▼';
        } else {
          icon.textContent = '';
        }
      });

      renderTable();
    });
  });

  // Modal close
  if (closeDetailModalBtn) closeDetailModalBtn.addEventListener('click', closeDetailModal);

  window.addEventListener('click', (e) => {
    if (e.target === detailModal) closeDetailModal();
  });

  window.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') closeDetailModal();
  });

  // Auto-refresh control
  function startAutoRefresh() {
    stopAutoRefresh();
    autoRefreshTimer = setInterval(() => {
      if (detailModal && detailModal.style.display === 'flex') return;
      fetchProcesses();
    }, 3000);
  }

  function stopAutoRefresh() {
    if (autoRefreshTimer) {
      clearInterval(autoRefreshTimer);
      autoRefreshTimer = null;
    }
  }

  if (autoRefreshCheckbox) {
    autoRefreshCheckbox.addEventListener('change', () => {
      if (autoRefreshCheckbox.checked) {
        startAutoRefresh();
      } else {
        stopAutoRefresh();
      }
    });
  }

  // Initial load
  fetchProcesses();
  if (autoRefreshCheckbox && autoRefreshCheckbox.checked) {
    startAutoRefresh();
  }
});
