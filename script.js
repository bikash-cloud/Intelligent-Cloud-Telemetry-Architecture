const cards = document.querySelectorAll(".card");
const steps = document.querySelectorAll(".step");
const techItems = document.querySelectorAll(".tech-grid div");

function hoverEffect(elements) {
  elements.forEach(item => {
    item.addEventListener("mouseenter", () => {
      item.style.transform = "translateY(-8px)";
    });

    item.addEventListener("mouseleave", () => {
      item.style.transform = "translateY(0)";
    });
  });
}

hoverEffect(cards);
hoverEffect(steps);
hoverEffect(techItems);

window.addEventListener("scroll", () => {
  const nav = document.querySelector("nav");

  if (window.scrollY > 50) {
    nav.style.boxShadow = "0 5px 25px rgba(0, 217, 255, 0.15)";
  } else {
    nav.style.boxShadow = "none";
  }
});

console.log("Intelligent Cloud Telemetry Architecture Website Loaded Successfully");
