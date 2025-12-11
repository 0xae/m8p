(function(){
    var MENU_HTML=`
    <nav class="flex-1 overflow-y-auto p-1.5 space-y-0.5">

        <a href="index.html" class="flex items-center gap-2 px-2 py-1.5 rounded-md text-textMuted hover:bg-surface hover:text-textMain transition-colors group">
            <i data-lucide="database" class="w-3.5 h-3.5 opacity-70 group-hover:opacity-100"></i>
            <span>M8 Console</span>
        </a>
        <div class="px-2 py-1.5 text-[9px] font-semibold uppercase tracking-wider text-textMuted">Engines</div>

        <a href="memory-engine.html" ng-click="app.setView('memory')" class="flex items-center gap-2 px-3 py-2 rounded-md text-textMuted hover:bg-surface hover:text-textMain transition-colors">
            <i data-lucide="history" class="w-4 h-4"></i>
            <span>Memory Engine</span>
        </a>

        <a href="recomendation-engine.html" class="flex items-center gap-2 px-3 py-2 rounded-md text-textMuted hover:bg-surface hover:text-textMain transition-colors">
            <i data-lucide="sparkles" class="w-4 h-4"></i>
            <span>Recomendation Engine</span>
        </a>

        <a href="stream-engine.html" class="flex items-center gap-2 px-3 py-2 rounded-md transition-colors group" 
            ng-class="'bg-purple-50 border border-purple-100 text-purple-600 font-medium' : 'text-textMuted hover:bg-surface'">
            <i data-lucide="activity" class="w-4 h-4"></i><span>Stream Engine</span>
        </a>

        <a href="visualizer.html" class="flex items-center gap-2 px-3 py-2 rounded-md text-textMuted hover:bg-surface hover:text-textMain transition-colors">
            <i data-lucide="zap" class="w-4 h-4"></i><span>
            VM Visualization</span>
        </a>

        <a href="Semantics-Physics-Engine.html"  
           class="flex items-center gap-2 px-3 py-2 rounded-md text-textMuted hover:bg-surface hover:text-textMain transition-colors">
            <i data-lucide="box" class="w-4 h-4"></i>
            <span>Holographic Sim</span>
        </a>
        <a href="Procedural-Geometry-Engine.html" 
           class="flex items-center gap-2 px-3 py-2 rounded-md text-textMuted hover:bg-surface hover:text-textMain transition-colors">
            <i data-lucide="hexagon" class="w-4 h-4"></i>
            <span>Procedural Geo</span>
        </a>

        <div class="px-2 py-1.5 mt-4 text-[9px] font-semibold uppercase tracking-wider text-textMuted">Financial Modules</div>

        <a href="market-analysis.html" class="flex items-center gap-2 px-3 py-2 rounded-md text-textMuted hover:bg-surface hover:text-textMain transition-colors opacity-60">
            <i data-lucide="trending-up" class="w-4 h-4"></i>
            <span>Market Overview</span>
        </a>

        <a href="futures-market.html" ng-click="app.setView('futures')" class="flex items-center gap-2 px-3 py-2 rounded-md text-textMuted hover:bg-surface hover:text-textMain transition-colors">
            <i data-lucide="bar-chart-2" class="w-4 h-4"></i>
            <span>Futures Analysis</span>
        </a>

    </nav>

    <div class="p-4 border-t border-border">
        <div class="text-[9px] text-textMuted flex items-center gap-1"><i data-lucide="wifi" class="w-3 h-3 text-green-500"></i> M8 Kernel: Online</div>
    </div>

    `;

     function M8_RENDER_MENU(elId, pageId) {
        var el = document.getElementById(elId);

        if (el) {
            el.innerHTML = MENU_HTML;
        }
    }

    window.RenderM8Menu = M8_RENDER_MENU;
})()