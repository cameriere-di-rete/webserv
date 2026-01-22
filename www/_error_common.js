// Shared behavior for static error pages.

(function () {
	try {
		document.addEventListener('DOMContentLoaded', function () {
			var codeEl = document.getElementById('errorCode');
			var titleEl = document.getElementById('errorTitle');
			var msgEl = document.getElementById('errorMsg');
			var birdElement = document.getElementById('theBird');
			var containerElement = document.getElementById('mainContainer');

			// Read text from the DOM (pages already render the text).
			// Fallback to defaults if the nodes are missing or empty.
			var code = (codeEl && codeEl.innerText) ? codeEl.innerText : '000';
			var title = (titleEl && titleEl.innerText) ? titleEl.innerText : 'Error';
			var msg = (msgEl && msgEl.innerText) ? msgEl.innerText : '';

			if (codeEl) codeEl.innerText = code;
			if (titleEl) titleEl.innerText = title;
			if (msgEl) msgEl.innerText = msg;

			// Interaction logic (copied from template, trimmed to essentials)
			var emojis = ['🍞', '🥐', '🥖', '🥨', '🥯', '🥞', '🧇', '🍰', '🥧', '🍩', '🍪', '🍮'];
			var birdRect, birdCenterX, birdCenterY;
			var jumpThreshold = 400;
			var mouseX = 0, mouseY = 0;
			var feedInterval = null, moveThrottle = null;

			function updateBirdPosition() {
				if (!birdElement) return;
				birdRect = birdElement.getBoundingClientRect();
				birdCenterX = birdRect.left + birdRect.width / 2;
				birdCenterY = birdRect.top + birdRect.height / 2;
			}

			window.addEventListener('resize', updateBirdPosition);
			updateBirdPosition();

			var scaleInterval = null; var scaleValue = 1; var maxScale = 1.7; var scaleStep = 0.02; var scaleDelay = 16;
			var boxElement = document.querySelector('.text-content');
			var originalMsg = null, originalTitle = null, originalBoxBg = null, originalMsgColor = null;

			function growBird() {
				if (scaleInterval) return;
				scaleInterval = setInterval(function () {
					if (scaleValue < maxScale) { scaleValue += scaleStep; if (scaleValue > maxScale) scaleValue = maxScale; birdElement.style.transform = 'scale(' + scaleValue + ')'; }
					if (scaleValue >= maxScale) {
						if (!originalMsg) originalMsg = msgEl.innerText;
						if (!originalTitle) originalTitle = titleEl.innerText;
						if (!originalBoxBg) originalBoxBg = boxElement.style.background;
						if (!originalMsgColor) originalMsgColor = msgEl.style.color;
						msgEl.innerText = 'facci passare webserv o ti mandiamo ' + code + ' piccioni a casa';
						titleEl.innerText = 'AO WAIT';
						boxElement.style.background = 'rgba(200,0,0,0.85)';
						msgEl.style.color = '#fff';
					}
				}, scaleDelay);
			}

			function shrinkBird() {
				if (scaleInterval) clearInterval(scaleInterval);
				if (originalMsg) { msgEl.innerText = originalMsg; originalMsg = null; }
				if (originalTitle) { titleEl.innerText = originalTitle; originalTitle = null; }
				boxElement.style.background = '';
				msgEl.style.color = '';
				scaleInterval = setInterval(function () {
					if (scaleValue > 1) { scaleValue -= scaleStep; if (scaleValue < 1) scaleValue = 1; birdElement.style.transform = 'scale(' + scaleValue + ')'; }
					else { clearInterval(scaleInterval); scaleInterval = null; }
				}, scaleDelay);
			}

			document.addEventListener('mousemove', function (e) {
				mouseX = e.pageX; mouseY = e.pageY;
				if (!moveThrottle) { moveThrottle = setTimeout(function () { createCrumb(mouseX, mouseY); moveThrottle = null; }, 40); }
				if (!birdElement) return;
				var dx = e.clientX - birdCenterX; var dy = e.clientY - birdCenterY; var distance = Math.sqrt(dx * dx + dy * dy);
				if (distance < jumpThreshold) {
					birdElement.classList.add('is-jumping'); containerElement.classList.add('bread-mode');
					if (!feedInterval) { feedInterval = setInterval(function () { createCrumb(mouseX, mouseY); }, 100); }
					growBird();
				} else {
					birdElement.classList.remove('is-jumping'); containerElement.classList.remove('bread-mode');
					if (feedInterval) { clearInterval(feedInterval); feedInterval = null; }
					shrinkBird();
				}
			});

			function createCrumb(x, y) {
				var el = document.createElement('span'); el.className = 'crumb'; el.innerText = emojis[Math.floor(Math.random() * emojis.length)];
				var dispersion = 40; var offsetX = (Math.random() - 0.5) * dispersion; var offsetY = (Math.random() - 0.5) * dispersion;
				el.style.left = (x + offsetX) + 'px'; el.style.top = (y + offsetY) + 'px'; el.style.fontSize = (20 + Math.random() * 15) + 'px';
				document.body.appendChild(el); setTimeout(function () { el.remove(); }, 1000);
			}
		});
	} catch (err) {
		console.error('ErrorCommon init failed', err);
	}
})();
