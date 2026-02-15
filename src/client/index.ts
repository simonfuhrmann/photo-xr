// Bundling the app requires esbuild only ("npm run bundle"), but ESBuild
// skips type checking. Type checking can be done with "npm run typecheck"
// using tsc manually. Usually, IDEs provide good type checking anyways.
import './components/xr-app';
