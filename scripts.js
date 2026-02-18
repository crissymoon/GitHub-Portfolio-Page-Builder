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

    /* Filter out empty projects (no title and no description) */
    var projects = [];
    for (var p = 0; p < data.projects.length; p++) {
      var proj = data.projects[p];
      if (proj.title || proj.description) {
        projects.push(proj);
      }
    }

    /* Sort by displayOrder */
    projects.sort(function (a, b) {
      var oa = (typeof a.displayOrder === "number") ? a.displayOrder : 9999;
      var ob = (typeof b.displayOrder === "number") ? b.displayOrder : 9999;
      return oa - ob;
    });

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

  /* ---- Rendering: Education & Technical Background ---- */

  function renderEducation(data) {
    var el = document.getElementById("education-block");
    var section = document.getElementById("education-section");
    if (!el || !section) return;
    if (!data.education || !data.education.showOnPortfolio || !data.education.summary) {
      section.style.display = "none";
      return;
    }
    section.style.display = "";
    var paragraphs = data.education.summary.split(/\n+/);
    var html = "";
    for (var p = 0; p < paragraphs.length; p++) {
      var trimmed = paragraphs[p].replace(/^\s+|\s+$/g, "");
      if (trimmed) {
        html += "<p>" + escapeHtml(trimmed) + "</p>";
      }
    }
    el.innerHTML = html;

    /* Trim and pull together animation on viewport enter */
    el.className = "education-block education-ready";
    if (typeof IntersectionObserver !== "undefined") {
      var observer = new IntersectionObserver(function (entries) {
        for (var i = 0; i < entries.length; i++) {
          if (entries[i].isIntersecting) {
            el.className = "education-block education-animate";
            observer.disconnect();
          }
        }
      }, { threshold: 0.1 });
      observer.observe(section);
    } else {
      el.className = "education-block";
    }
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

  /* ---- Rendering: Social Links ---- */

  function renderSocials(data) {
    var el = document.getElementById("socials-bar");
    if (!el) return;
    if (!data.socials || !data.socials.length) {
      el.innerHTML = "";
      el.style.display = "none";
      return;
    }
    el.style.display = "";
    var html = "";
    for (var i = 0; i < data.socials.length; i++) {
      var s = data.socials[i];
      if (s.platform && s.url) {
        html += '<li><a href="' + escapeHtml(s.url) + '" target="_blank" rel="noopener">' + escapeHtml(s.platform) + "</a></li>";
      }
    }
    el.innerHTML = html;
  }

  /* ---- Rendering: Support / Buy Me a Coffee ---- */

  function renderSupportLink(data) {
    var el = document.getElementById("support-link-wrap");
    if (!el) return;
    if (!data.supportLink || !data.supportLink.url) {
      el.innerHTML = "";
      el.style.display = "none";
      return;
    }
    el.style.display = "";
    var label = data.supportLink.label || "Buy This Dev a Cup of Coffee";
    el.innerHTML = '<a class="support-link" href="' + escapeHtml(data.supportLink.url) + '" target="_blank" rel="noopener">' + escapeHtml(label) + "</a>";
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
    renderSocials(data);
    renderUpdates(data);
    renderProjects(data);
    renderInfo(data);
    renderSkills(data);
    renderEducation(data);
    renderLinks(data);
    renderSupportLink(data);
  }

  function init() {
    loadData(function (err, data) {
      if (err) {
        var main = document.querySelector(".main-content");
        if (main) main.innerHTML = "<p>Error loading portfolio data.</p>";
        hideLoadingScreen();
        return;
      }
      renderAll(data);
      hideLoadingScreen();
    });
  }

  function hideLoadingScreen() {
    var screen = document.getElementById("loading-screen");
    if (!screen) return;
    screen.className = "loading-screen loading-fade-out";
    setTimeout(function () {
      if (screen.parentNode) {
        screen.parentNode.removeChild(screen);
      }
    }, 450);
  }

  /* Wait for DOM */
  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
