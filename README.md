# How to run the app

This is the SUST APP. We use SvelteKit as the framework and surrealDB as the database.

## Installing surrealDB
We used surrealDB as the database. So, you must have it installed.

```bash
iwr https://windows.surrealdb.com -useb | iex
```

Make sure you have NodeJS installed.

## Developing

After every push and clone, you must install the dependencies.

```bash
npm install
```
Run the surrealDB server:
```bash
npm run db
```

We have some data loaded in a file. To import the data (In separate terminal):
```bash
npm run load
```

Once you have installed the dependencies, you can start the development server (In separate terminal):

```bash
npm run dev

# or start the server and open the app in a new browser tab
npm run dev -- --open
```

If you have changed the data, you can export it to the file:
```bash
npm run export
```

Remember to commit the changes to the file to Github.

# Deployment

If you are just working with it and testing, you don't have to use the following steps. But if you want to deploy it, you must follow the following steps.

## Building

To create a production version of your app:

```bash
npm run build
```

You can preview the production build with `npm run preview`.

> To deploy your app, you may need to install an [adapter](https://kit.svelte.dev/docs/adapters) for your target environment.
