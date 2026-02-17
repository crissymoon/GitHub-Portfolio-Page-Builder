/* Crissy Portfolio - scripts.js */
(function () {
  "use strict";

  var PROJECTS_PER_PAGE = 3;
  var currentPage = 1;
  var siteData = null;

  /* ---- Data Loading ---- */

  function loadData(callback) {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "crissy-data.json?v=" + Date.now(), true);
    xhr.onreadystatechange = function () {
      if (xhr.readyState === 4) {
        if (xhr.status === 200) {
          try {
            siteData = JSON.parse(xhr.responseText);
            callback(null, siteData);
          } catch (e) {
            callback(e, null);
          }
        } else {
          callback(new Error("Failed to load data"), null);
        }
      }
    };
    xhr.send();
  }

  /* ---- Rendering: Notice ---- */

  function renderNotice(data) {
    var el = document.getElementById("notice-bar");
    if (!el || !data.notice) return;
    var html = data.notice.text;
    if (data.notice.link) {
      html = html.replace(
        data.notice.link,
        '<a href="' + escapeHtml(data.notice.link) + '">' + escapeHtml(data.notice.link) + "</a>"
      );
    }
    el.innerHTML = html;
  }

  /* ---- Rendering: Header ---- */

  function renderHeader(data) {
    var el = document.getElementById("site-title");
    if (!el || !data.site) return;
    el.textContent = data.site.title;
  }

  /* ---- Rendering: Image & Favicon ---- */

  function renderImage(data) {
    var el = document.getElementById("site-image");
    if (!el) return;
    if (data.site && data.site.image) {
      el.src = data.site.image;
      el.alt = (data.site.title || "Portfolio") + " image";
      el.style.display = "block";

      /* Apply border settings */
      if (data.site.imageBorder) {
        var color = data.site.imageBorderColor || "#ffffff";
        el.style.border = "2px solid " + color;
      } else {
        el.style.border = "none";
      }

      /* Apply size class */
      el.className = "site-image";
      var size = data.site.imageSize || "medium";
      if (size === "small") el.className += " site-image-small";
      else if (size === "large") el.className += " site-image-large";
      else el.className += " site-image-medium";
    } else {
      el.style.display = "none";
    }
  }

  function renderFavicon(data) {
    var el = document.getElementById("favicon");
    if (!el) return;
    if (data.site && data.site.image) {
      el.href = data.site.image;
    }
  }

  /* ---- Rendering: Updates ---- */

  function renderUpdates(data) {
    var el = document.getElementById("update-notice");
    if (!el || !data.notice) return;
    var html = data.notice.text;
    if (data.notice.link) {
      html = html.replace(
        data.notice.link,
        '<a href="' + escapeHtml(data.notice.link) + '">' + escapeHtml(data.notice.link) + "</a>"
      );
    }
    el.innerHTML = html;
  }

  /* ---- Rendering: Projects (Paginated) ---- */

  function renderProjects(data) {
    var container = document.getElementById("projects-grid");
    if (!container || !data.projects) return;

    var projects = data.projects;
    var totalPages = Math.ceil(projects.length / PROJECTS_PER_PAGE);
    if (currentPage > totalPages) currentPage = totalPages;
    if (currentPage < 1) currentPage = 1;

    var start = (currentPage - 1) * PROJECTS_PER_PAGE;
    var end = start + PROJECTS_PER_PAGE;
    var pageProjects = projects.slice(start, end);

    var html = "";
    for (var i = 0; i < pageProjects.length; i++) {
      var p = pageProjects[i];
      html += '<div class="project-card">';
      html += "<h3>" + escapeHtml(p.title) + "</h3>";
      html += "<p>" + escapeHtml(p.description) + "</p>";
      if (p.tags && p.tags.length) {
        html += '<div class="project-tags">';
        for (var t = 0; t < p.tags.length; t++) {
          html += '<span class="project-tag">' + escapeHtml(p.tags[t]) + "</span>";
        }
        html += "</div>";
      }
      if (p.url) {
        html += '<a class="project-link" href="' + escapeHtml(p.url) + '" target="_blank" rel="noopener">View Project</a>';
      }
      html += "</div>";
    }
    container.innerHTML = html;

    renderPagination(totalPages);
  }

  function renderPagination(totalPages) {
    var el = document.getElementById("pagination");
    if (!el) return;

    if (totalPages <= 1) {
      el.innerHTML = "";
      return;
    }

    var html = "";
    html += '<button id="prev-btn"' + (currentPage <= 1 ? " disabled" : "") + ">Prev</button>";
    html += '<span class="page-info">Page ' + currentPage + " of " + totalPages + "</span>";
    html += '<button id="next-btn"' + (currentPage >= totalPages ? " disabled" : "") + ">Next</button>";
    el.innerHTML = html;

    var prevBtn = document.getElementById("prev-btn");
    var nextBtn = document.getElementById("next-btn");

    if (prevBtn) {
      prevBtn.onclick = function () {
        if (currentPage > 1) {
          currentPage--;
          renderProjects(siteData);
        }
      };
    }
    if (nextBtn) {
      nextBtn.onclick = function () {
        var total = Math.ceil(siteData.projects.length / PROJECTS_PER_PAGE);
        if (currentPage < total) {
          currentPage++;
          renderProjects(siteData);
        }
      };
    }
  }

  /* ---- Rendering: Info & Skills ---- */

  function renderInfo(data) {
    var el = document.getElementById("info-block");
    if (!el || !data.info) return;

    var html = "";
    html += "<p><strong>" + escapeHtml(data.info.name) + "</strong></p>";
    if (data.info.email) {
      html += '<p>Email: <a href="mailto:' + escapeHtml(data.info.email) + '">' + escapeHtml(data.info.email) + "</a></p>";
    }
    if (data.info.bio) {
      html += "<p>" + escapeHtml(data.info.bio) + "</p>";
    }
    el.innerHTML = html;
  }

  function renderSkills(data) {
    var el = document.getElementById("skills-list");
    if (!el || !data.info || !data.info.skills) return;

    var html = "";
    for (var i = 0; i < data.info.skills.length; i++) {
      html += "<li>" + escapeHtml(data.info.skills[i]) + "</li>";
    }
    el.innerHTML = html;
  }

  /* ---- Rendering: Links ---- */

  function renderLinks(data) {
    var el = document.getElementById("links-list");
    if (!el || !data.links) return;

    var html = "";
    for (var i = 0; i < data.links.length; i++) {
      var lnk = data.links[i];
      html += '<li><a href="' + escapeHtml(lnk.url) + '">' + escapeHtml(lnk.label) + "</a></li>";
    }
    el.innerHTML = html;
  }

  /* ---- Rendering: CSS Variables from JSON ---- */

  function applyStyles(data) {
    if (!data.styles) return;
    var root = document.documentElement;
    var s = data.styles;
    if (s.primary) root.style.setProperty("--primary", s.primary);
    if (s.secondary) root.style.setProperty("--secondary", s.secondary);
    if (s.accent) root.style.setProperty("--accent", s.accent);
    if (s.text) root.style.setProperty("--text", s.text);
    if (s.background) root.style.setProperty("--background", s.background);
    if (s.harmony && s.harmony.length >= 3) {
      root.style.setProperty("--harmony-1", s.harmony[0]);
      root.style.setProperty("--harmony-2", s.harmony[1]);
      root.style.setProperty("--harmony-3", s.harmony[2]);
    }
    if (s.bodyFont) root.style.setProperty("--body-font", s.bodyFont);
    if (s.headingFont) root.style.setProperty("--heading-font", s.headingFont);
    if (s.headingWeight) root.style.setProperty("--heading-weight", s.headingWeight);
    if (s.baseSize) root.style.setProperty("--base-size", s.baseSize);
    if (s.borderRadius) root.style.setProperty("--border-radius", s.borderRadius);
  }

  /* ---- Utility ---- */

  function escapeHtml(str) {
    if (!str) return "";
    var div = document.createElement("div");
    div.appendChild(document.createTextNode(str));
    return div.innerHTML;
  }

  /* ---- Init ---- */

  function renderAll(data) {
    applyStyles(data);
    renderNotice(data);
    renderHeader(data);
    renderImage(data);
    renderFavicon(data);
    renderUpdates(data);
    renderProjects(data);
    renderInfo(data);
    renderSkills(data);
    renderLinks(data);
  }

  function init() {
    loadData(function (err, data) {
      if (err) {
        var main = document.querySelector(".main-content");
        if (main) main.innerHTML = "<p>Error loading portfolio data.</p>";
        return;
      }
      renderAll(data);
    });
  }

  /* Wait for DOM */
  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
