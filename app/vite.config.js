import { defineConfig } from "vite"
import preact from "@preact/preset-vite"
import { viteSingleFile } from "vite-plugin-singlefile"
import { ViteMinifyPlugin } from "vite-plugin-minify"

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [preact({
    babel: {
      plugins: [
        ["@emotion/babel-plugin", { "sourceMap": false }],
      ],
    },
  }), viteSingleFile(), ViteMinifyPlugin({})],
})
