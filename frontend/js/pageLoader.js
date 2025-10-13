/**
 * Page Loader Utility
 * Handles dynamic loading of page HTML without modifying existing JS files
 */

class PageLoader {
    constructor() {
        this.currentPage = 'dashboard';
        this.cache = {}; // Cache loaded pages for performance
        this.loadingIndicator = null;
    }

    /**
     * Initialize the page loader
     */
    init() {
        // Create loading indicator
        this.createLoadingIndicator();
        
        // Load initial page (dashboard)
        this.loadPage('dashboard');
        
        // Setup navigation listeners
        this.setupNavigation();
    }

    /**
     * Create a loading indicator element
     */
    createLoadingIndicator() {
        this.loadingIndicator = document.createElement('div');
        this.loadingIndicator.className = 'page-loading';
        this.loadingIndicator.innerHTML = '<i class="fas fa-spinner fa-spin"></i>';
        this.loadingIndicator.style.cssText = `
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            font-size: 2rem;
            color: var(--primary-color);
            display: none;
        `;
    }

    /**
     * Setup navigation click handlers
     */
    setupNavigation() {
        const navItems = document.querySelectorAll('.nav-item');
        
        navItems.forEach(item => {
            item.addEventListener('click', (e) => {
                e.preventDefault();
                const pageName = item.getAttribute('data-page');
                
                // Update active state
                navItems.forEach(nav => nav.classList.remove('active'));
                item.classList.add('active');
                
                // Load the page
                this.loadPage(pageName);
            });
        });
    }

    /**
     * Load a page dynamically
     * @param {string} pageName - Name of the page to load
     */
    async loadPage(pageName) {
        if (this.currentPage === pageName && this.cache[pageName]) {
            this.showPage(pageName);
            return;
        }

        try {
            // Show loading indicator
            this.showLoading();

            // Check cache first
            let html;
            if (this.cache[pageName]) {
                html = this.cache[pageName];
            } else {
                // Fetch the page HTML
                const response = await fetch(`pages/${pageName}.html`);
                if (!response.ok) {
                    throw new Error(`Failed to load page: ${pageName}`);
                }
                html = await response.text();
                
                // Cache the page
                this.cache[pageName] = html;
            }

            // Update the page content
            const pageContainer = document.getElementById(`page-${pageName}`);
            if (pageContainer) {
                pageContainer.innerHTML = html;
            }

            // Hide all pages
            document.querySelectorAll('.page').forEach(page => {
                page.classList.remove('active');
                page.style.display = 'none';
            });

            // Show the requested page
            if (pageContainer) {
                pageContainer.style.display = 'block';
                pageContainer.classList.add('active');
            }

            // Update current page
            this.currentPage = pageName;

            // Hide loading indicator
            this.hideLoading();

            // Trigger page-specific initialization if needed
            this.triggerPageInit(pageName);

        } catch (error) {
            console.error('Error loading page:', error);
            this.hideLoading();
            this.showError(pageName);
        }
    }

    /**
     * Show a specific page from cache
     * @param {string} pageName 
     */
    showPage(pageName) {
        document.querySelectorAll('.page').forEach(page => {
            page.classList.remove('active');
            page.style.display = 'none';
        });

        const pageContainer = document.getElementById(`page-${pageName}`);
        if (pageContainer) {
            pageContainer.style.display = 'block';
            pageContainer.classList.add('active');
        }

        this.currentPage = pageName;
        this.triggerPageInit(pageName);
    }

    /**
     * Show loading indicator
     */
    showLoading() {
        if (this.loadingIndicator) {
            document.querySelector('.main-content').appendChild(this.loadingIndicator);
            this.loadingIndicator.style.display = 'block';
        }
    }

    /**
     * Hide loading indicator
     */
    hideLoading() {
        if (this.loadingIndicator) {
            this.loadingIndicator.style.display = 'none';
        }
    }

    /**
     * Show error message
     * @param {string} pageName 
     */
    showError(pageName) {
        const pageContainer = document.getElementById(`page-${pageName}`);
        if (pageContainer) {
            pageContainer.innerHTML = `
                <div class="error-message" style="text-align: center; padding: 3rem;">
                    <i class="fas fa-exclamation-triangle" style="font-size: 3rem; color: var(--warning-color);"></i>
                    <h2>Error Loading Page</h2>
                    <p>Could not load the ${pageName} page. Please try again.</p>
                </div>
            `;
            pageContainer.style.display = 'block';
            pageContainer.classList.add('active');
        }
    }

    /**
     * Trigger page-specific initialization
     * This allows existing JS files to reinitialize their content
     * @param {string} pageName 
     */
    triggerPageInit(pageName) {
        // Dispatch custom event that existing JS can listen to
        const event = new CustomEvent('pageLoaded', {
            detail: { page: pageName }
        });
        document.dispatchEvent(event);

        // For graphs page, reinitialize charts after DOM is ready
        if (pageName === 'graphs') {
            setTimeout(() => {
                // Check if chartManager exists and reinitialize charts
                if (window.chartManager) {
                    console.log('Reinitializing charts for graphs page...');
                    
                    // Destroy existing charts to avoid duplicates
                    if (window.chartManager.charts) {
                        Object.keys(window.chartManager.charts).forEach(key => {
                            if (key.includes('Chart') && window.chartManager.charts[key]) {
                                try {
                                    window.chartManager.charts[key].destroy();
                                    delete window.chartManager.charts[key];
                                } catch (e) {
                                    console.warn('Error destroying chart:', key, e);
                                }
                            }
                        });
                    }
                    
                    // Reinitialize main sensor charts
                    const chartConfigs = [
                        { id: 'tempChart', label: 'Temperature', unit: '°C', color: '#ef4444' },
                        { id: 'humidChart', label: 'Humidity', unit: '%', color: '#3b82f6' },
                        { id: 'pressureChart', label: 'Pressure', unit: 'Pa', color: '#8b5cf6' },
                        { id: 'luxChart', label: 'Light', unit: 'lux', color: '#fbbf24' },
                        { id: 'gasChart', label: 'Gas', unit: 'PPM', color: '#10b981' }
                    ];
                    
                    chartConfigs.forEach(config => {
                        if (document.getElementById(config.id)) {
                            window.chartManager.createSensorChart(
                                config.id, 
                                config.label, 
                                config.unit, 
                                config.color
                            );
                        }
                    });
                    
                    // Trigger data update if dashboard instance exists
                    if (window.dashboard && window.dashboard.updateGraphs) {
                        window.dashboard.updateGraphs();
                    }
                }
            }, 100);
        }
        
        // For predictions page, reinitialize prediction chart
        if (pageName === 'predictions') {
            setTimeout(() => {
                if (window.chartManager && document.getElementById('predictionHistoryChart')) {
                    console.log('Reinitializing prediction history chart...');
                    
                    // Destroy existing prediction chart
                    if (window.chartManager.charts['predictionHistoryChart']) {
                        try {
                            window.chartManager.charts['predictionHistoryChart'].destroy();
                            delete window.chartManager.charts['predictionHistoryChart'];
                        } catch (e) {
                            console.warn('Error destroying prediction chart:', e);
                        }
                    }
                    
                    // Recreate prediction history chart
                    window.chartManager.createPredictionHistoryChart();
                }
            }, 100);
        }
    }

    /**
     * Preload a page
     * @param {string} pageName 
     */
    async preload(pageName) {
        if (!this.cache[pageName]) {
            try {
                const response = await fetch(`pages/${pageName}.html`);
                if (response.ok) {
                    this.cache[pageName] = await response.text();
                }
            } catch (error) {
                console.warn(`Failed to preload page: ${pageName}`, error);
            }
        }
    }

    /**
     * Preload common pages for better UX
     */
    preloadCommonPages() {
        const commonPages = ['graphs', 'predictions', 'activity'];
        commonPages.forEach(page => this.preload(page));
    }
}

// Initialize page loader when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    window.pageLoader = new PageLoader();
    window.pageLoader.init();
    
    // Preload common pages after initial load
    setTimeout(() => {
        window.pageLoader.preloadCommonPages();
    }, 1000);
});
