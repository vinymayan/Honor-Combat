import { defineConfig } from 'vite'
import solid from 'vite-plugin-solid'

export default defineConfig({
    plugins: [solid()],
    base: './',
    build: {
        target: 'es2022',
        assetsDir: '',
        rollupOptions: {
            output: {
                // Remove os hashes [hash] dos nomes dos arquivos
                entryFileNames: `[name].js`,
                chunkFileNames: `[name].js`,
                assetFileNames: `[name][extname]`
            }
        }
    }
})
